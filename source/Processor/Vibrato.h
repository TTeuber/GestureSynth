//
// Created by Tyler Teuber on 2/10/26.
//

#pragma once

#include <cmath>
#include <juce_core/juce_core.h>

class Vibrato
{
public:
    void prepare (float newSampleRate)
    {
        sampleRate = newSampleRate;
        phaseIncrement = rate / sampleRate;
    }

    void setRate (float hz)
    {
        rate = hz;
        if (sampleRate > 0.0f)
            phaseIncrement = rate / sampleRate;
    }

    void setDepth (float semitones) { depth = semitones; }

    void setEnabled (bool shouldBeEnabled) { enabled = shouldBeEnabled; }

    void reset() { phase = 0.0f; }

    float getFrequencyMultiplier (int numSamples)
    {
        if (!enabled || depth == 0.0f)
            return 1.0f;

        const float lfoValue = std::sin (juce::MathConstants<float>::twoPi * phase);
        phase += phaseIncrement * static_cast<float> (numSamples);
        while (phase >= 1.0f)
            phase -= 1.0f;

        return std::pow (2.0f, depth * lfoValue / 12.0f);
    }

private:
    float sampleRate = 44100.0f;
    float rate = 5.0f;
    float depth = 0.0f;
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    bool enabled = false;
};
