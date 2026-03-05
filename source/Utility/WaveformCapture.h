#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>

class WaveformCapture
{
public:
    static constexpr int kBufferSize = 4096;

    void write (const float* data, int numSamples)
    {
        int wp = writePos.load (std::memory_order_relaxed);
        for (int i = 0; i < numSamples; ++i)
        {
            buffer[static_cast<size_t> (wp)] = data[i];
            wp = (wp + 1) % kBufferSize;
        }
        writePos.store (wp, std::memory_order_release);
    }

    bool read (float* dest, int maxSamples) const
    {
        const int wp = writePos.load (std::memory_order_acquire);
        const int numToRead = juce::jmin (maxSamples, kBufferSize);

        int readStart = (wp - numToRead + kBufferSize) % kBufferSize;
        for (int i = 0; i < numToRead; ++i)
            dest[i] = buffer[static_cast<size_t> ((readStart + i) % kBufferSize)];

        for (int i = numToRead; i < maxSamples; ++i)
            dest[i] = 0.0f;

        return true;
    }

    std::array<float, kBufferSize> buffer {};
    std::atomic<int> writePos { 0 };
};
