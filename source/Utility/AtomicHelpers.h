#pragma once

#include <atomic>

/**
 * Helpers for atomic parameter access between UI and audio threads.
 *
 * This codebase uses two atomic ordering patterns:
 *
 * 1. **Relaxed** (paramLoad / paramStore) — for independent parameter values
 *    (knobs, toggles, sliders). Each value is self-contained; no other memory
 *    operations depend on observing the store in a particular order.
 *
 * 2. **Acquire/Release** — for lock-free data structures (LockFreeQueue,
 *    WaveformCapture) where a producer's writes to a buffer must be visible
 *    to the consumer before the index update is observed. These use explicit
 *    memory_order_acquire / memory_order_release and are NOT wrapped here.
 */
namespace AtomicHelpers
{

template <typename T>
inline T paramLoad (const std::atomic<T>& a) noexcept
{
    return a.load (std::memory_order_relaxed);
}

template <typename T>
inline void paramStore (std::atomic<T>& a, T value) noexcept
{
    a.store (value, std::memory_order_relaxed);
}

template <typename T>
inline T paramExchange (std::atomic<T>& a, T value) noexcept
{
    return a.exchange (value, std::memory_order_relaxed);
}

} // namespace AtomicHelpers
