# Thread Safety in Audio Plugins: A Practical Guide

## The Problem: UI and Audio Thread Conflicts

Audio plugins run on multiple threads simultaneously:

| Thread | Responsibility | Priority |
|--------|---------------|----------|
| **Audio Thread** | Real-time audio processing (`renderNextBlock`) | Highest - must never block |
| **UI Thread** | User interface, parameter changes, ValueTree callbacks | Normal |

When both threads access the same data structure without synchronization, you get **race conditions** - unpredictable behavior that causes crashes, glitches, or corrupted data.

## What Went Wrong

In `MySynthVoice`, we had this pattern:

```cpp
// UI Thread - called when user moves a slider
void MySynthVoice::valueTreePropertyChanged(...) {
    modMatrix.updateModulation(source, destination, depth);  // Modifies std::map
}

// Audio Thread - called 44,100+ times per second
void MySynthVoice::renderNextBlock(...) {
    modMatrix.processSample();  // Iterates std::map
}
```

### The Race Condition

```
Time →
─────────────────────────────────────────────────────────────────────

UI Thread:     [─── updateModulation() ───]
                    ↓ modifies map

Audio Thread:            [─── processSample() ───]
                              ↓ iterates map

               ════════════════════════════════
                    CRASH! Iterator invalidated
```

When the UI thread modifies the `std::map` while the audio thread is iterating it:
- Iterators become invalid
- Memory gets corrupted
- The plugin crashes (especially in hosts like Logic Pro with strict thread checking)

## Why Mutexes Don't Work Here

The obvious solution might be:

```cpp
std::mutex mtx;

void updateModulation(...) {
    std::lock_guard<std::mutex> lock(mtx);  // BLOCKS until available
    // modify...
}

void processSample() {
    std::lock_guard<std::mutex> lock(mtx);  // BLOCKS until available
    // iterate...
}
```

**This is dangerous for audio!** If the UI thread holds the lock, the audio thread blocks waiting for it. This causes:
- Audio dropouts (glitches, pops, clicks)
- Buffer underruns
- The dreaded "spinning beachball" in the DAW

**Rule: The audio thread must NEVER block.**

## The Solution: Lock-Free Command Queue

Instead of direct modification, we queue commands from the UI thread and process them on the audio thread at safe points.

```
UI Thread                              Audio Thread
    │                                       │
    │  valueTreePropertyChanged()           │
    │         │                             │
    │         ▼                             │
    │  ┌──────────────────┐                 │
    │  │ commandQueue.push │ ──────────────►│  renderNextBlock()
    │  │   (lock-free)     │                │         │
    │  └──────────────────┘                 │         ▼
    │                                       │  processPendingCommands()
    │                                       │         │
    │                                       │         ▼
    │                                       │  modMatrix.processSample()
    │                                       │
```

### How Lock-Free Queues Work

A **Single Producer Single Consumer (SPSC)** ring buffer uses atomic operations instead of locks:

```cpp
template<typename T, size_t Capacity>
class LockFreeQueue {
    std::array<T, Capacity> buffer;
    std::atomic<size_t> readIndex{0};   // Only audio thread modifies
    std::atomic<size_t> writeIndex{0};  // Only UI thread modifies

public:
    // UI thread only
    bool push(const T& item) noexcept {
        size_t currentWrite = writeIndex.load(std::memory_order_relaxed);
        size_t nextWrite = (currentWrite + 1) % Capacity;

        if (nextWrite == readIndex.load(std::memory_order_acquire))
            return false;  // Queue full

        buffer[currentWrite] = item;
        writeIndex.store(nextWrite, std::memory_order_release);
        return true;
    }

    // Audio thread only
    bool pop(T& item) noexcept {
        size_t currentRead = readIndex.load(std::memory_order_relaxed);

        if (currentRead == writeIndex.load(std::memory_order_acquire))
            return false;  // Queue empty

        item = buffer[currentRead];
        readIndex.store((currentRead + 1) % Capacity, std::memory_order_release);
        return true;
    }
};
```

Key properties:
- **No locks** - uses atomic operations with memory barriers
- **No blocking** - `push()` and `pop()` return immediately
- **Thread-safe** - each index is only written by one thread

## Implementation Pattern

### 1. Define Command Structure

```cpp
enum class ModCommandType { Add, Remove, Update };

struct ModCommand {
    ModCommandType type;
    ModSource* source;
    ModDestination* destination;
    float depth;
    bool isBipolar;
};
```

### 2. Add Queue to Your Class

```cpp
class ModMatrix {
    // ... existing members ...
    LockFreeQueue<ModCommand, 16> commandQueue;  // 16 slots is usually plenty
};
```

### 3. Create Thread-Safe Queue Methods

```cpp
// Called from UI thread - safe to call anytime
void ModMatrix::queueUpdateModulation(ModSource* source, ModDestination* dest, float depth) {
    ModCommand cmd{ModCommandType::Update, source, dest, depth, false};
    commandQueue.push(cmd);  // Non-blocking, lock-free
}
```

### 4. Process Queue at Block Boundaries

```cpp
// Called from audio thread at the START of each block
void ModMatrix::processPendingCommands() noexcept {
    ModCommand cmd;
    while (commandQueue.pop(cmd)) {
        switch (cmd.type) {
            case ModCommandType::Add:
                addModulation(cmd.destination, cmd.source, cmd.depth, cmd.isBipolar);
                break;
            case ModCommandType::Remove:
                removeModulation(cmd.source, cmd.destination);
                break;
            case ModCommandType::Update:
                updateModulation(cmd.source, cmd.destination, cmd.depth);
                break;
        }
    }
}
```

### 5. Update Callbacks to Use Queue

```cpp
void MySynthVoice::valueTreePropertyChanged(...) {
    auto* source = modSources[sourceId];
    auto* destination = modDestinations[destId];

    if (source == nullptr || destination == nullptr)
        return;  // Always validate!

    modMatrix.queueUpdateModulation(source, destination, depth);  // Queue, don't call directly
}
```

### 6. Process at Block Start

```cpp
void MySynthVoice::renderNextBlock(...) {
    modMatrix.processPendingCommands();  // First thing! Before any processing

    // ... rest of audio processing ...
}
```

## Quick Reference: What's Safe Where?

### Audio Thread (renderNextBlock, processSample, etc.)
- **DO:** Read from atomic variables
- **DO:** Pop from lock-free queues
- **DO:** Modify audio-thread-owned data
- **DON'T:** Allocate memory (`new`, `malloc`, `std::vector::push_back`)
- **DON'T:** Lock mutexes
- **DON'T:** Call virtual functions on objects owned by UI thread
- **DON'T:** Access STL containers being modified elsewhere

### UI Thread (ValueTree callbacks, parameter listeners, etc.)
- **DO:** Push to lock-free queues
- **DO:** Modify UI-owned data
- **DO:** Allocate memory (if needed)
- **DON'T:** Directly modify audio-thread data structures
- **DON'T:** Assume your changes are immediately visible to audio thread

## Common Patterns That Cause Issues

### 1. Direct Container Modification
```cpp
// BAD - UI thread modifies, audio thread reads
std::vector<Voice*> voices;
void addVoice(Voice* v) { voices.push_back(v); }  // UI thread
void process() { for (auto* v : voices) ... }      // Audio thread

// GOOD - Queue the change
void addVoice(Voice* v) { voiceQueue.push({Add, v}); }
void process() {
    processVoiceCommands();  // Apply changes safely
    for (auto* v : voices) ...
}
```

### 2. Shared Pointers Across Threads
```cpp
// RISKY - reference count modifications aren't always lock-free
std::shared_ptr<Data> data;
void updateData(std::shared_ptr<Data> d) { data = d; }  // UI thread
void process() { auto local = data; ... }                // Audio thread

// SAFER - use atomic operations or queue the pointer
std::atomic<Data*> data;
// or
LockFreeQueue<Data*, 4> dataQueue;
```

### 3. String Operations
```cpp
// BAD - strings allocate memory
juce::String currentPreset;
void loadPreset(const String& name) { currentPreset = name; }  // UI
void process() { if (currentPreset == "heavy") ... }           // Audio

// GOOD - use enums or indices
enum class Preset { Light, Heavy };
std::atomic<Preset> currentPreset;
```

## Testing for Thread Safety

### 1. Thread Sanitizer (TSan)
Build with `-fsanitize=thread`:
```bash
# In CMakeLists.txt
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(YourPlugin PRIVATE -fsanitize=thread)
    target_link_options(YourPlugin PRIVATE -fsanitize=thread)
endif()
```

### 2. Stress Testing
- Rapidly move sliders while playing complex MIDI
- Automate parameters while playing
- Test in Logic Pro (strict thread checking)
- Test with small buffer sizes (32-64 samples)

### 3. JUCE Assertions
Enable `JUCE_STRICT_REFCOUNTEDPOINTER` and check for assertion failures.

## Summary

| Situation | Solution |
|-----------|----------|
| UI needs to modify audio data | Lock-free queue |
| Audio needs to notify UI | Lock-free queue or atomic flag |
| Shared read-only data | `const` references, set once at init |
| Shared mutable data | Atomic types for simple values |
| Complex shared state | Copy-on-write or double buffering |

**Remember:** When in doubt, queue it out. The audio thread's job is to process audio as fast as possible - everything else can wait.
