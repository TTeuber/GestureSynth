//
// Created by Tyler Teuber on 4/9/25.
//

#pragma once

#include "PitchTracker.h"
#include "PluginProcessor.h"
#include "Theme.h"

#include <juce_audio_utils/juce_audio_utils.h>

class Oscilloscope final : public juce::Component, public juce::Timer
{
public:
    explicit Oscilloscope (PluginProcessor& p) : processor (p), pitchTracker (p.pitchTracker)
    {
        waveform.resize (1024, 0.0f);
        startTimerHz (6); // Update at 30 FPS
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (SECONDARY_COLOR);
        g.setColour (TEXT_COLOR);

        juce::Path wavePath;
        const auto width = static_cast<float> (getWidth());
        const auto height = static_cast<float> (getHeight());
        const float halfHeight = height / 2.0f;

        wavePath.startNewSubPath (0, halfHeight);

        int numSamples = 2 * static_cast<int> (processor.getSampleRate() / pitchTracker->frequency);
        while (numSamples > waveform.size())
            numSamples /= 2;

        bool zeroCrossing = false;
        float xOffset = 0;
        for (int i = 0; i < numSamples; ++i)
        {
            // if (std::abs (waveform[i] > 0.0001f || waveform[i] < waveform[(i + 10) % numSamples]) && !zeroCrossing)
            // {
            //     xOffset += 1;
            //     continue;
            // }
            // else
            //     zeroCrossing = true;
            // const float x = static_cast<float> (i) / static_cast<float> (waveform.size()) * width; // Scale to width
            const float x = static_cast<float> (i) / static_cast<float> (numSamples) * width - xOffset; // Scale to width
            const float y = halfHeight - waveform[i] * halfHeight * 5.0f; // Scale amplitude
            wavePath.lineTo (x, y);
        }
        g.strokePath (wavePath, juce::PathStrokeType (1.0f));
    }

    void timerCallback() override
    {
        if (processor.getWaveformData (waveform.data(), static_cast<int> (waveform.size())))
        {
            repaint();
        }
    }

private:
    PluginProcessor& processor;
    std::vector<float> waveform;
    std::shared_ptr<PitchTracker> pitchTracker;
};
