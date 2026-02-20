//
// Created by Tyler Teuber on 4/9/25.
//

#pragma once

#include "../PluginProcessor.h"
#include "../Theme.h"
#include "../Utility/PitchTracker.h"

#include <juce_audio_utils/juce_audio_utils.h>

class Oscilloscope final : public juce::Component, public juce::Timer
{
public:
    explicit Oscilloscope (PluginProcessor& p) : processor (p), pitchTracker (p.pitchTracker)
    {
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (SECONDARY_COLOR);
        g.setColour (TEXT_COLOR);

        const auto width = static_cast<float> (getWidth());
        const auto height = static_cast<float> (getHeight());
        const float halfHeight = height / 2.0f;

        if (displaySamples <= 0)
        {
            // No valid data — draw flat line
            juce::Path flatPath;
            flatPath.startNewSubPath (0, halfHeight);
            flatPath.lineTo (width, halfHeight);
            g.strokePath (flatPath, juce::PathStrokeType (1.0f));
            return;
        }

        juce::Path wavePath;
        wavePath.startNewSubPath (0, halfHeight);

        for (int i = 0; i < displaySamples; ++i)
        {
            const float x = static_cast<float> (i) / static_cast<float> (displaySamples) * width;
            const float y = halfHeight - displayBuffer[static_cast<size_t> (i)] * halfHeight;
            wavePath.lineTo (x, y);
        }

        g.strokePath (wavePath, juce::PathStrokeType (1.0f));
    }

    void timerCallback() override
    {
        const double sampleRate = processor.getSampleRate();
        const float freq = pitchTracker->frequency;

        if (sampleRate <= 0.0 || freq <= 0.0f)
        {
            displaySamples = 0;
            repaint();
            return;
        }

        // Snapshot the circular buffer
        const int wp = processor.waveWritePos.load (std::memory_order_acquire);
        std::array<float, PluginProcessor::kWaveBufferSize> buf {};
        for (int i = 0; i < PluginProcessor::kWaveBufferSize; ++i)
            buf[static_cast<size_t> (i)] = processor.waveData[static_cast<size_t> (i)];

        // Calculate display window: ~2 periods
        const auto samplesPerPeriod = static_cast<int> (std::ceil (sampleRate / static_cast<double> (freq)));
        int numPeriods = 2;

        // If 2 periods don't fit in the buffer, halve until they do
        while (numPeriods > 0 && samplesPerPeriod * numPeriods + samplesPerPeriod > PluginProcessor::kWaveBufferSize)
            numPeriods /= 2;

        if (numPeriods <= 0 || samplesPerPeriod <= 0)
        {
            displaySamples = 0;
            repaint();
            return;
        }

        const int windowSamples = samplesPerPeriod * numPeriods;
        const int searchMargin = samplesPerPeriod; // one period of search margin

        // Total samples we need to look back from write head
        const int totalNeeded = windowSamples + searchMargin;
        if (totalNeeded > PluginProcessor::kWaveBufferSize)
        {
            displaySamples = 0;
            repaint();
            return;
        }

        // Search start index in the circular buffer
        const int searchStart = (wp - totalNeeded + PluginProcessor::kWaveBufferSize) % PluginProcessor::kWaveBufferSize;

        // Find rising zero-crossing in the search margin
        int triggerIdx = -1;
        for (int i = 0; i < searchMargin; ++i)
        {
            const int idx = (searchStart + i) % PluginProcessor::kWaveBufferSize;
            const int idxNext = (searchStart + i + 1) % PluginProcessor::kWaveBufferSize;
            if (buf[static_cast<size_t> (idx)] <= 0.0f && buf[static_cast<size_t> (idxNext)] > 0.0f)
            {
                triggerIdx = (searchStart + i + 1) % PluginProcessor::kWaveBufferSize;
                break;
            }
        }

        // Fallback: no trigger found, use untriggered display from end of search margin
        if (triggerIdx < 0)
            triggerIdx = (searchStart + searchMargin) % PluginProcessor::kWaveBufferSize;

        // Copy display window into displayBuffer
        displaySamples = windowSamples;
        for (int i = 0; i < windowSamples; ++i)
        {
            const int idx = (triggerIdx + i) % PluginProcessor::kWaveBufferSize;
            displayBuffer[static_cast<size_t> (i)] = buf[static_cast<size_t> (idx)];
        }

        repaint();
    }

private:
    PluginProcessor& processor;
    std::shared_ptr<PitchTracker> pitchTracker;

    std::array<float, PluginProcessor::kWaveBufferSize> displayBuffer {};
    int displaySamples = 0;
};
