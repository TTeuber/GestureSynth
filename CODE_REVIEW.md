# SynthDemo Code Review

## Executive Summary

This is a well-architected JUCE synthesizer VST with thoughtful design for modulation and voice management. However, **critical thread safety issues** are almost certainly causing the Logic Pro crashes. The codebase shows good understanding of synth architecture but needs fixes in three key areas before continuing development.

---

## Project Overview

| Aspect | Details                                                        |
|--------|----------------------------------------------------------------|
| Framework | JUCE 8.x                                                       |
| Type | Subtractive Synthesizer VST                                    |
| Voices | 8 polyphonic                                                   |
| Features | Dual oscillators, LFO, ADSR, Filter, Modulation Matrix, Chorus |
| UI Size | 1000x800 pixels                                                |

### Architecture Summary
```
MIDI → PluginProcessor → MySynth (8 voices)
                              ↓
                         MySynthVoice
                         ├─ JuneDCO (PolyBLEP oscillator)
                         ├─ StateVariableTPTFilter
                         ├─ MyADSR (envelope)
                         ├─ MyLFO (modulation)
                         └─ ModMatrix (routing)
                              ↓
                         Output → JuneChorus → Audio Out
```

---

## Critical Issues (Likely Crash Causes)

### 1. Unsafe Voice Pointer Pattern

**Location:** `PluginProcessor.cpp:175-186`, `MySynthVoice.cpp:162-163`

**The Problem:**
```cpp
// Current implementation - problematic
std::shared_ptr<MySynthVoice*> voicePtr = std::make_shared<MySynthVoice*>(nullptr);

// In MySynthVoice::startNote():
*voicePtr = this;  // Sets pointer to active voice

// In PluginProcessor::processBlock():
if (*voicePtr != nullptr) {
    const auto& voiceBuffer = (*voicePtr)->getWaveformBuffer();  // DANGER
}
```

**Why it crashes:**
- Creates `shared_ptr<pointer>` instead of `shared_ptr<MySynthVoice>`
- When a voice ends, the pointer becomes dangling
- Rapid MIDI events cause race conditions between voices
- Logic Pro's aggressive MIDI processing exposes this more than other DAWs

**Fix:**
```cpp
// Option 1: Simple atomic pointer
std::atomic<MySynthVoice*> activeVoice{nullptr};

// Option 2: Lock-free queue for waveform data instead of pointer sharing
```

---

### 2. Waveform Buffer Race Condition

**Location:** `MySynthVoice.cpp:77`, `PluginProcessor.cpp:177-186`

**The Problem:**
```cpp
// Audio thread writes:
waveformBuffer.writeBuffer(outputBuffer, outputBuffer.getNumSamples());

// UI thread reads (via Oscilloscope timer):
const auto& voiceBuffer = (*voicePtr)->getWaveformBuffer();
waveData[i] = voiceBuffer.getSample(0, i);
```

**Why it crashes:**
- No synchronization between read/write operations
- `AbstractFifo` is declared but never used
- UI thread can read partially written data

**Fix:**
```cpp
// Use the existing waveFifo properly
// Write with FIFO protection:
auto [start1, size1, start2, size2] = waveFifo.write(numSamples);
// Copy data to appropriate ranges...

// Read with FIFO protection:
auto [start1, size1, start2, size2] = waveFifo.read(numSamples);
```

---

### 3. ValueTree Listener on Audio Thread

**Location:** `MySynthVoice.h:22`, `MySynthVoice.cpp:15,116-132`

**The Problem:**
```cpp
class MySynthVoice : public juce::SynthesiserVoice,
                     public juce::ValueTree::Listener  // Called from UI thread!
{
    // In constructor:
    modTree.addListener(this);  // Registers for callbacks

    // This gets called when UI changes modulation:
    void valueTreePropertyChanged(...) override {
        // UNSAFE: Modifies modMatrix while audio thread uses it
        modMatrix.updateModulation(source, destination, depth);
    }
};
```

**Why it crashes:**
- UI thread modifies modulation routing
- Audio thread iterates over the same data structures
- No synchronization = memory corruption

**Fix:**
```cpp
// Option 1: Queue changes for audio thread to process
std::atomic<bool> modRoutingChanged{false};
ModRoutingUpdate pendingUpdate;

// In listener callback (UI thread):
pendingUpdate = {source, destination, depth};
modRoutingChanged.store(true);

// In renderNextBlock (audio thread):
if (modRoutingChanged.exchange(false)) {
    modMatrix.updateModulation(pendingUpdate...);
}
```

---

## Major Issues

### 4. Null Pointer Access in ModMatrix

**Location:** `Modulation/Modulation.cpp:48-64`

```cpp
void ModMatrix::processSample() const noexcept
{
    for (const auto& [destination, mods] : matrix)
    {
        float value = destination->getRawParameterValue();  // No null check!
        // ...
    }
}
```

If `modSources[sourceId]` or `modDestinations[destinationId]` returns nullptr in the listener callback, it gets stored and later dereferenced.

---

### 5. Memory Leak in MatrixComponent

**Location:** `Editor/MatrixComponent.h:16-34`

```cpp
// Sliders manually allocated
auto* rotarySlider = new juce::Slider(...);
rotarySliders.push_back(rotarySlider);

// Manual deletion in destructor - error prone
~MatrixComponent() {
    for (const auto* rotarySlider : rotarySliders)
        delete rotarySlider;
}
```

**Fix:** Use `std::unique_ptr` or JUCE's `OwnedArray`:
```cpp
juce::OwnedArray<juce::Slider> rotarySliders;
```

---

### 6. Parameter Read Mid-Block

**Location:** `MySynthVoice.cpp:58`

```cpp
void MySynthVoice::renderNextBlock(...)
{
    // Reading parameter during block processing
    volume = parameters.getParameter("volume")->getValue();

    for (int sample = 0; sample < numSamples; ++sample) {
        tempDataL[sample] *= volume;  // Same value for all samples
    }
}
```

Should use the cached value from `MySynth::updateParameters()` instead.

---

### 7. Filter State Not Reset

**Location:** `MySynthVoice.cpp:50-55`

When filter is disabled and re-enabled, it retains previous state which can cause clicks/pops.

---

## Moderate Issues

### 8. Oscillator Oversampling Calculation Error

**Location:** `Processor/JuneOscillator.h:114-120`

```cpp
// Current:
phaseIncrement = this->frequency / sampleRate;

// Should be:
float oversampledSampleRate = sampleRate * oversampling.factorOversampling;
phaseIncrement = frequency / oversampledSampleRate;
```

---

### 9. LFOData Listener Thread Safety

**Location:** `Modulation/LFOData.h:51-61`

`listeners` vector is modified without lock, but `updateListeners()` iterates it from potentially different thread.

---

### 10. Inefficient Parameter Updates

**Location:** `MySynth.cpp:35-50`

Creates a new `std::vector` every audio block. Should be cached at initialization.

---

### 11. Member Variable Shadowing

**Location:** `Utility/MyParameter.h:82,116`

`DynamicParameter::parameter` shadows `MyParameter::parameter`.

---

## Minor Issues

### 12. Oscilloscope Update Rate
`startTimerHz(6)` is very slow. Should be 30-60 Hz for smooth display.

### 13. Unused Code
- `JuneChorus` declared but not used in voice
- `AntiAliasOscillator.h` commented out
- `juneOscillator2` appears unused in rendering

### 14. No Denormal Protection in Voice
`juce::ScopedNoDenormals` used in processBlock but not in individual voice rendering.

---

## Recommended Fix Priority

### Immediate (Before any other development)
1. Fix voicePtr pattern - Replace with atomic or remove entirely
2. Add waveform buffer synchronization - Use AbstractFifo properly
3. Fix ValueTree listener threading - Queue changes for audio thread

### Short Term (Next sprint)
4. Add null checks to ModMatrix
5. Use smart pointers in MatrixComponent
6. Cache parameter values properly
7. Reset filter state when toggling

### Medium Term (Quality improvements)
8. Fix oversampling calculation
9. Add LFOData thread safety
10. Optimize parameter update pattern
11. Remove unused code

---

## Code Quality Observations

### What's Good
- Clean separation of concerns (Processor, Synthesizer, Modulation, Editor folders)
- Proper use of JUCE's StateVariableTPTFilter
- PolyBLEP anti-aliasing implementation
- Modular UI component design
- Parameter system is well-thought-out with StaticParameter/DynamicParameter split

### What Needs Work
- Thread safety throughout
- Memory management consistency (mix of raw/smart pointers)
- Some JUCE best practices not followed
- Dead code and incomplete features

---

## Suggested Development Path

1. **Stabilization Phase**
   - Fix the three critical issues
   - Test in Logic Pro until stable
   - Add basic error handling

2. **Cleanup Phase**
   - Remove unused code
   - Standardize memory management
   - Add null checks

3. **Feature Completion**
   - Enable chorus effect
   - Complete second oscillator integration
   - Polish modulation matrix

4. **Polish Phase**
   - Improve oscilloscope
   - Add presets
   - UI refinements

---

## Files to Focus On

| File | Priority | Reason |
|------|----------|--------|
| `PluginProcessor.cpp` | CRITICAL | Voice pointer and waveform issues |
| `MySynthVoice.cpp` | CRITICAL | Thread safety, parameter reads |
| `Modulation/Modulation.cpp` | HIGH | Null pointer risks |
| `Editor/MatrixComponent.h` | HIGH | Memory management |
| `MySynth.cpp` | MEDIUM | Parameter update efficiency |
| `Processor/JuneOscillator.h` | MEDIUM | Oversampling calculation |

---

*Generated: 2026-02-04*
