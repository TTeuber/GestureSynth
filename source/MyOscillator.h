//
// Created by Tyler Teuber on 3/19/25.
//

#pragma once
#include <juce_dsp/juce_dsp.h>

class MyOscillator : public juce::dsp::Oscillator<float>
{
public:
    enum Waveform {
        Sine,
        Saw,
        Square,
        Triangle
    };
    explicit MyOscillator (const Waveform wave = Saw)
    {
        setWaveform (wave);
    }

    void setWaveform (Waveform newWaveform);
    void setPulseWidth (const float newPulseWidth) { pulseWidth = newPulseWidth; }

    static float sineWave (const float x)
    {
        return juce::jmap<float> (std::sin (x),
            -juce::MathConstants<double>::pi,
            juce::MathConstants<double>::pi,
            -1,
            1);
    }
    static float sawWave (const float x)
    {
        return juce::jmap<float> (x,
            -juce::MathConstants<double>::pi,
            juce::MathConstants<double>::pi,
            -1,
            1);
    }

    [[nodiscard]] float squareWave (const float x) const
    {
        return juce::jmap<float> (
            x < pulseWidth * juce::MathConstants<float>::twoPi - juce::MathConstants<float>::pi ? -1.0f : 1.0f,
            -juce::MathConstants<double>::pi,
            juce::MathConstants<double>::pi,
            -1,
            1);
    }

    static float triangleWave (float x)
    {
        return juce::jmap<float> (
            2.0f * (std::abs (x / juce::MathConstants<double>::pi) - 1),
            -juce::MathConstants<double>::pi,
            juce::MathConstants<double>::pi,
            -1,
            1);
    }

    float pulseWidth = 0.5f;
};
