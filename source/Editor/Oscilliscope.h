//
// Created by Tyler Teuber on 4/9/25.
//

#pragma once

#include "../PluginProcessor.h"
#include "../Theme.h"
#include "../Utility/Parameters.h"
#include "../Utility/PitchTracker.h"

#include <juce_audio_utils/juce_audio_utils.h>
#include <limits>

class Oscilloscope final : public juce::Component, public juce::Timer
{
public:
    explicit Oscilloscope (PluginProcessor& p) : processor (p), pitchTracker (p.pitchTracker)
    {
        subOnParam = dynamic_cast<juce::AudioParameterBool*> (p.parameters.getParameter (ParamIDs::subOn));
        subLevelParam = p.parameters.getParameter (ParamIDs::subOsc);
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
        float freq = pitchTracker->frequency;

        // When the sub oscillator is active, the composite waveform's
        // fundamental period is the sub osc period (one octave lower = half freq)
        const bool subActive = subOnParam != nullptr && subOnParam->get()
            && subLevelParam != nullptr && subLevelParam->getValue() > 0.0f;
        if (subActive)
            freq *= 0.5f;

        if (sampleRate <= 0.0 || freq <= 0.0f)
        {
            displaySamples = 0;
            repaint();
            return;
        }

        // Snapshot the circular buffer
        auto& capture = processor.waveformCapture;
        const int wp = capture.writePos.load (std::memory_order_acquire);
        std::array<float, WaveformCapture::kBufferSize> buf {};
        for (int i = 0; i < WaveformCapture::kBufferSize; ++i)
            buf[static_cast<size_t> (i)] = capture.buffer[static_cast<size_t> (i)];

        // Calculate display window: ~2 periods
        const auto samplesPerPeriod = static_cast<int> (std::ceil (sampleRate / static_cast<double> (freq)));
        int numPeriods = 2;

        // If 2 periods don't fit in the buffer, halve until they do
        while (numPeriods > 0 && samplesPerPeriod * numPeriods + samplesPerPeriod > WaveformCapture::kBufferSize)
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
        if (totalNeeded > WaveformCapture::kBufferSize)
        {
            displaySamples = 0;
            repaint();
            return;
        }

        // Search start index in the circular buffer
        const int searchStart = (wp - totalNeeded + WaveformCapture::kBufferSize) % WaveformCapture::kBufferSize;

        // Find rising zero-crossing in the search margin
        int triggerIdx = -1;

        if (subActive)
        {
            // With the sub oscillator active, there are two rising zero-crossings
            // per sub period (one per main osc cycle). They differ in the sub osc
            // phase, causing the display to flip between two views. Pick the
            // crossing followed by the most positive energy to consistently lock
            // onto the sub oscillator's positive half-cycle.
            const int mainPeriodSamples = samplesPerPeriod / 2;
            const int scoreLen = juce::jmax (1, mainPeriodSamples / 2);
            float bestScore = -std::numeric_limits<float>::max();

            for (int i = 0; i < searchMargin; ++i)
            {
                const int idx = (searchStart + i) % WaveformCapture::kBufferSize;
                const int idxNext = (searchStart + i + 1) % WaveformCapture::kBufferSize;
                if (buf[static_cast<size_t> (idx)] <= 0.0f && buf[static_cast<size_t> (idxNext)] > 0.0f)
                {
                    const int candidate = (searchStart + i + 1) % WaveformCapture::kBufferSize;
                    float score = 0.0f;
                    for (int j = 0; j < scoreLen; ++j)
                    {
                        const int si = (candidate + j) % WaveformCapture::kBufferSize;
                        score += buf[static_cast<size_t> (si)];
                    }
                    if (score > bestScore)
                    {
                        bestScore = score;
                        triggerIdx = candidate;
                    }
                }
            }
        }
        else
        {
            for (int i = 0; i < searchMargin; ++i)
            {
                const int idx = (searchStart + i) % WaveformCapture::kBufferSize;
                const int idxNext = (searchStart + i + 1) % WaveformCapture::kBufferSize;
                if (buf[static_cast<size_t> (idx)] <= 0.0f && buf[static_cast<size_t> (idxNext)] > 0.0f)
                {
                    triggerIdx = (searchStart + i + 1) % WaveformCapture::kBufferSize;
                    break;
                }
            }
        }

        // Fallback: no trigger found, use untriggered display from end of search margin
        if (triggerIdx < 0)
            triggerIdx = (searchStart + searchMargin) % WaveformCapture::kBufferSize;

        // Copy display window into displayBuffer
        displaySamples = windowSamples;
        for (int i = 0; i < windowSamples; ++i)
        {
            const int idx = (triggerIdx + i) % WaveformCapture::kBufferSize;
            displayBuffer[static_cast<size_t> (i)] = buf[static_cast<size_t> (idx)];
        }

        repaint();
    }

private:
    PluginProcessor& processor;
    std::shared_ptr<PitchTracker> pitchTracker;
    juce::AudioParameterBool* subOnParam = nullptr;
    juce::RangedAudioParameter* subLevelParam = nullptr;

    std::array<float, WaveformCapture::kBufferSize> displayBuffer {};
    int displaySamples = 0;
};
