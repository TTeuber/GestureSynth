//
// Created by Tyler Teuber on 3/19/25.
//

#pragma once
#include <juce_dsp/juce_dsp.h>

static float polyBlep (float t, const float dt)
{
    if (t < dt)
    {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    if (t > 1.0f - dt)
    {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

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
    [[nodiscard]] float sawWave (const float x) const
    {
        constexpr float period = juce::MathConstants<float>::twoPi;
        float t = x / period;
        t -= std::floor (t); // wrap to [0, 1)
        float value = 2.0f * t - 1.0f; // Raw saw wave
        // Apply polyBLEP correction
        const float dt = 1.0f / getSampleRate(); // You'll need to add getSampleRate() to your class
        value -= polyBlep (t, dt);
        return value;

        // return juce::jmap<float> (x,
        //     -juce::MathConstants<double>::pi,
        //     juce::MathConstants<double>::pi,
        //     -1,
        //     1);
    }

    [[nodiscard]] float getSampleRate() const
    {
        return sampleRate;
    }

    [[nodiscard]] float squareWave (const float x) const
    {
        // constexpr float period = juce::MathConstants<float>::twoPi;
        // float t = x / period;
        // t -= std::floor (t); // wrap to [0, 1)
        // // Calculate the pulse width threshold
        // const float pwThreshold = pulseWidth;
        // // Generate raw square wave
        // float value = t < pwThreshold ? -1.0f : 1.0f;
        //
        // const float dt = 1.0f / getSampleRate();
        //
        // // Apply polyBLEP at both transitions
        // // Rising edge at t = pwThreshold
        // value += polyBlep (fmod (t - pwThreshold, 1.0f), dt);
        // // Falling edge at t = 0
        // value -= polyBlep (t, dt);
        //
        // return value;

        return juce::jmap<float> (
            x < pulseWidth * juce::MathConstants<float>::twoPi - juce::MathConstants<float>::pi ? -1.0f : 1.0f,
            -juce::MathConstants<double>::pi,
            juce::MathConstants<double>::pi,
            -1,
            1);
    }

    static float triangleWave (const float x)
    {
        return juce::jmap<float> (
            2.0f * (std::abs (x / juce::MathConstants<double>::pi) - 1),
            -juce::MathConstants<double>::pi,
            juce::MathConstants<double>::pi,
            -1,
            1);
    }

    float pulseWidth = 0.5f;
    float sampleRate = 44100.0f;
};
