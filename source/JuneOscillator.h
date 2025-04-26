//
// Created by Tyler Teuber on 4/25/25.
//

#pragma once
#include "MyParameter.h"
#include <juce_dsp/juce_dsp.h>

class JuneDCO
{
public:
    JuneDCO (DynamicParameter& pw)
        : pulseWidth (pw)
    {
        // Initialize oversampling (2x, stereo, IIR filter for efficiency)

        reset();
    }

    // Prepare oscillator for playback
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        oversampling.initProcessing (spec.maximumBlockSize);
        // numSamples = spec.maximumBlockSize;

        reset();
    }

    // Reset phase and state
    void reset()
    {
        phase = 0.0;
        subPhase = 0.0;
    }

    // Set frequency in Hz
    void setFrequency (const float frequency)
    {
        this->frequency = frequency / std::pow (2, oversampling.factorOversampling);
        phaseIncrement = this->frequency / sampleRate;
        subPhaseIncrement = phaseIncrement * 0.5; // Sub-oscillator is one octave lower
    }

    // Set pulse width (0.0 to 1.0)
    // void setPulseWidth (float width)
    // {
    //     pulseWidth = juce::jlimit (0.01f, 0.99f, width);
    // }

    // Set mix levels (0.0 to 1.0)
    void setWaveformMix (float sawLevel, float pulseLevel, float subLevel)
    {
        this->sawLevel = juce::jlimit (0.0f, 1.0f, sawLevel);
        this->pulseLevel = juce::jlimit (0.0f, 1.0f, pulseLevel);
        this->subLevel = juce::jlimit (0.0f, 1.0f, subLevel);
    }

    // PolyBLEP function to reduce aliasing
    static float polyBlep (float t, const float dt)
    {
        // t is the phase position (0 to 1)
        // dt is the phase increment (normalized frequency)

        // Handle discontinuity at the beginning of the waveform
        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.0f;
        }
        // Handle discontinuity at the end of the waveform
        if (t > 1.0 - dt)
        {
            t = (t - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        }
        // No discontinuity
        return 0.0f;
    }

    // Process a single sample
    float process()
    {
        // Store previous phase for discontinuity detection
        float previousPhase = phase;
        float previousSubPhase = subPhase;

        // Update phase (0.0 to 1.0)
        phase += phaseIncrement;
        if (phase >= 1.0)
            phase -= 1.0;

        // Update sub-oscillator phase
        subPhase += subPhaseIncrement;
        if (subPhase >= 1.0)
            subPhase -= 1.0;

        // Generate band-limited sawtooth wave using PolyBLEP
        float saw = 2.0f * phase - 1.0f; // Naive sawtooth: -1 to 1

        // Apply PolyBLEP correction at discontinuity
        // For sawtooth, there's one discontinuity when the phase wraps
        if (previousPhase > phase) // Phase wrapped around
            saw -= polyBlep (phase, phaseIncrement);

        // Generate band-limited pulse wave using PolyBLEP
        float pulse = (phase < pulseWidth.getCurrentValue()) ? 1.0 : -1.0; // Naive pulse wave

        // Apply PolyBLEP correction at rising and falling edges
        if (phase < phaseIncrement) // Rising edge
            pulse += polyBlep (phase, phaseIncrement);
        if (phase > pulseWidth.getCurrentValue() && phase < pulseWidth.getCurrentValue() + phaseIncrement) // Falling edge
            pulse -= polyBlep (phase - pulseWidth.getCurrentValue(), phaseIncrement);

        // Generate band-limited sub-oscillator (square wave with 50% duty cycle)
        float sub = (subPhase < 0.5) ? 1.0 : -1.0; // Naive square wave

        // Apply PolyBLEP correction for sub-oscillator
        if (subPhase < subPhaseIncrement) // Rising edge
            sub += polyBlep (subPhase, subPhaseIncrement);
        if (subPhase > 0.5 && subPhase < 0.5 + subPhaseIncrement) // Falling edge
            sub -= polyBlep (subPhase - 0.5, subPhaseIncrement);

        // Mix waveforms
        float output = saw * sawLevel + pulse * pulseLevel + sub * subLevel;

        // Normalize output to prevent clipping
        float mixSum = sawLevel + pulseLevel + subLevel;
        if (mixSum > 0.0)
            output /= mixSum;

        return output;
    }

    // Process a block of samples
    void processBlock (juce::dsp::AudioBlock<float>& buffer)
    {
        auto osBuffer = oversampling.processSamplesUp (buffer);

        for (int i = 0; i < osBuffer.getNumSamples(); ++i)
        {
            const float val = process();
            osBuffer.getChannelPointer (0)[i] = val;
            osBuffer.getChannelPointer (1)[i] = val; // Stereo output
        }
        oversampling.processSamplesDown (buffer);
    }

private:
    float sampleRate = 44100.0;
    float frequency = 440.0;
    float phase = 0.0;
    float phaseIncrement = 0.0;
    float subPhase = 0.0;
    float subPhaseIncrement = 0.0;
    DynamicParameter& pulseWidth;
    float sawLevel = 0.0f;
    float pulseLevel = 1.0f;
    float subLevel = 0.0f;
    // size_t numSamples = 512;
    juce::dsp::Oversampling<float> oversampling = juce::dsp::Oversampling<float> (
        2, // Stereo
        2, // 2x oversampling
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
        true, // High-quality upsampling
        false // Normal-quality downsampling
    );
};