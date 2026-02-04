//
// Lock-free SPSC (Single Producer Single Consumer) ring buffer
// Used for thread-safe communication between UI and audio threads
//

#pragma once

#include <array>
#include <atomic>
#include <cstddef>

template <typename T, size_t Capacity>
class LockFreeQueue
{
public:
    LockFreeQueue() = default;

    // Push an item to the queue (UI thread only)
    // Returns true if successful, false if queue is full
    bool push (const T& item) noexcept
    {
        const size_t currentWrite = writeIndex.load (std::memory_order_relaxed);
        const size_t nextWrite = (currentWrite + 1) % Capacity;

        // Check if queue is full
        if (nextWrite == readIndex.load (std::memory_order_acquire))
            return false;

        buffer[currentWrite] = item;
        writeIndex.store (nextWrite, std::memory_order_release);
        return true;
    }

    // Pop an item from the queue (audio thread only)
    // Returns true if successful, false if queue is empty
    bool pop (T& item) noexcept
    {
        const size_t currentRead = readIndex.load (std::memory_order_relaxed);

        // Check if queue is empty
        if (currentRead == writeIndex.load (std::memory_order_acquire))
            return false;

        item = buffer[currentRead];
        readIndex.store ((currentRead + 1) % Capacity, std::memory_order_release);
        return true;
    }

    // Check if queue is empty (can be called from any thread)
    [[nodiscard]] bool isEmpty() const noexcept
    {
        return readIndex.load (std::memory_order_acquire) == writeIndex.load (std::memory_order_acquire);
    }

private:
    std::array<T, Capacity> buffer {};
    std::atomic<size_t> readIndex { 0 };
    std::atomic<size_t> writeIndex { 0 };
};
