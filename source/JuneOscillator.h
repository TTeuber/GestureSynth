//
// Created by Tyler Teuber on 4/25/25.
//

#pragma once
#include "MyParameter.h"
#include <juce_dsp/juce_dsp.h>

class JuneDCO : public juce::AudioProcessorValueTreeState::Listener
{
public:
    JuneDCO (juce::AudioProcessorValueTreeState& p, DynamicParameter& pw)
        : parameters (p), pulseWidth (pw)
    {
        parameters.addParameterListener ("oscWaveform", this);
        parameters.addParameterListener ("oscDetune", this);
        parameters.addParameterListener ("oscWidth", this);
        parameters.addParameterListener ("subOsc", this);

        reset();
    }
    ~JuneDCO() override
    {
        parameters.removeParameterListener ("oscWaveform", this);
    }

    void parameterChanged (const juce::String& parameterID, float newValue) override
    {
        if (parameterID == "oscWaveform")
        {
            // Handle waveform change if needed
            sawLevel = 1.0f - newValue;
            pulseLevel = newValue;
        }
        else if (parameterID == "oscDetune")
        {
            // Handle detune change if needed
            detune = newValue;
            frequencyL = frequency * std::pow (2, detune * detuneAmount / 12.0f);
            frequencyR = frequency * std::pow (2, -(detune * detuneAmount) / 12.0f);
            phaseIncrementL = frequencyL / sampleRate;
            phaseIncrementR = frequencyR / sampleRate;
        }
        else if (parameterID == "oscWidth")
        {
            stereoWidth = newValue;
        }
        else if (parameterID == "subOsc")
        {
            // Handle sub-oscillator level change if needed
            subLevel = newValue;
        }
    }

    // Prepare oscillator for playback
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = static_cast<float> (spec.sampleRate);
        oversampling.initProcessing (spec.maximumBlockSize);
        // numSamples = spec.maximumBlockSize;

        reset();
    }

    // Reset phase and state
    void reset()
    {
        // phase = 0.0;
        phaseL = 0.0;
        phaseR = 0.0;
        subPhase = 0.0;
    }

    // Set frequency in Hz
    void setFrequency (const float frequency)
    {
        this->frequency = frequency / std::pow (2, oversampling.factorOversampling);
        frequencyL = this->frequency * std::pow (2, detune * detuneAmount / 12.0f);
        frequencyR = this->frequency * std::pow (2, -(detune * detuneAmount) / 12.0f);
        // phaseIncrement = this->frequency / sampleRate;
        phaseIncrementL = frequencyL / sampleRate;
        phaseIncrementR = frequencyR / sampleRate;
        subPhaseIncrement = this->frequency / sampleRate / 2; // Sub-oscillator is one octave lower
    }

    // Set mix levels (0.0 to 1.0)
    void setWaveformMix (const float sawLevel, const float pulseLevel, const float subLevel)
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
    std::array<float, 2> process()
    {
        // Store previous phase for discontinuity detection
        // float previousPhase = phase;
        float previousPhaseL = phaseL;
        float previousPhaseR = phaseR;
        float previousSubPhase = subPhase;

        // Update phase (0.0 to 1.0)
        // phase += phaseIncrement;
        phaseL += phaseIncrementL;
        phaseR += phaseIncrementR;
        // if (phase >= 1.0)
        //     phase -= 1.0;
        if (phaseL >= 1.0)
            phaseL -= 1.0;
        if (phaseR >= 1.0)
            phaseR -= 1.0;

        // Update sub-oscillator phase
        subPhase += subPhaseIncrement;
        if (subPhase >= 1.0)
            subPhase -= 1.0;

        // Generate band-limited sawtooth wave using PolyBLEP
        // float saw = 2.0f * phase - 1.0f; // Naive sawtooth: -1 to 1
        float sawL = 2.0f * phaseL - 1.0f; // Naive sawtooth: -1 to 1
        float sawR = 2.0f * phaseR - 1.0f; // Naive sawtooth: -1 to 1

        // Apply PolyBLEP correction at discontinuity
        // For sawtooth, there's one discontinuity when the phase wraps
        // if (previousPhase > phase) // Phase wrapped around
        //     saw -= polyBlep (phase, phaseIncrement);
        if (previousPhaseL > phaseL) // Phase wrapped around
            sawL -= polyBlep (phaseL, phaseIncrementL);
        if (previousPhaseR > phaseR) // Phase wrapped around
            sawR -= polyBlep (phaseR, phaseIncrementR);

        // Generate band-limited pulse wave using PolyBLEP
        // float pulse = (phase < pulseWidth.getCurrentValue()) ? -1.0 : 1.0; // Naive pulse wave
        float pulseL = (phaseL < pulseWidth.getCurrentValue()) ? -1.0 : 1.0; // Naive pulse wave
        float pulseR = (phaseR < pulseWidth.getCurrentValue()) ? -1.0 : 1.0; // Naive pulse wave

        // Apply PolyBLEP correction at rising and falling edges
        // if (phase < phaseIncrement) // Rising edge
        //     pulse += polyBlep (phase, phaseIncrement);
        // if (phase > pulseWidth.getCurrentValue() && phase < pulseWidth.getCurrentValue() + phaseIncrement) // Falling edge
        //     pulse -= polyBlep (phase - pulseWidth.getCurrentValue(), phaseIncrement);
        if (phaseL < phaseIncrementL) // Rising edge
            pulseL += polyBlep (phaseL, phaseIncrementL);
        if (phaseL > pulseWidth.getCurrentValue() && phaseL < pulseWidth.getCurrentValue() + phaseIncrementL) // Falling edge
            pulseL -= polyBlep (phaseL - pulseWidth.getCurrentValue(), phaseIncrementL);
        if (phaseR < phaseIncrementR) // Rising edge
            pulseR += polyBlep (phaseR, phaseIncrementR);
        if (phaseR > pulseWidth.getCurrentValue() && phaseR < pulseWidth.getCurrentValue() + phaseIncrementR) // Falling edge
            pulseR -= polyBlep (phaseR - pulseWidth.getCurrentValue(), phaseIncrementR);

        // Generate band-limited sub-oscillator (square wave with 50% duty cycle)
        float sub = (subPhase < 0.5) ? 1.0 : -1.0; // Naive square wave

        // Apply PolyBLEP correction for sub-oscillator
        if (subPhase < subPhaseIncrement) // Rising edge
            sub += polyBlep (subPhase, subPhaseIncrement);
        if (subPhase > 0.5 && subPhase < 0.5 + subPhaseIncrement) // Falling edge
            sub -= polyBlep (subPhase - 0.5, subPhaseIncrement);

        // Mix waveforms
        // float output = saw * sawLevel + pulse * pulseLevel + sub * subLevel;
        float outputL = sawL * sawLevel + pulseL * pulseLevel + sub * subLevel;
        float outputR = sawR * sawLevel + pulseR * pulseLevel + sub * subLevel;

        // // Normalize output to prevent clipping
        const float mixSum = sawLevel + pulseLevel + subLevel;
        if (mixSum > 0.0)
        {
            // output /= mixSum;
            outputL /= mixSum;
            outputR /= mixSum;
        }

        return { outputL, outputR };
    }

    // Process a block of samples
    void processBlock (juce::dsp::AudioBlock<float>& buffer)
    {
        auto osBuffer = oversampling.processSamplesUp (buffer);

        if (detune == 0.0f)
        {
            for (int i = 0; i < osBuffer.getNumSamples(); ++i)
            {
                const std::array<float, 2> vals = process();
                osBuffer.getChannelPointer (0)[i] = vals[0]; // Mono output
                osBuffer.getChannelPointer (1)[i] = vals[0]; // Stereo output
            }
        }
        else
        {
            for (int i = 0; i < osBuffer.getNumSamples(); ++i)
            {
                const std::array<float, 2> vals = process();
                osBuffer.getChannelPointer (0)[i] = vals[0] * stereoWidth + vals[1] * (1.0f - stereoWidth); // Mono output
                osBuffer.getChannelPointer (1)[i] = vals[1] * stereoWidth + vals[0] * (1.0f - stereoWidth); // Stereo output
            }
        }
        oversampling.processSamplesDown (buffer);
    }

private:
    juce::AudioProcessorValueTreeState& parameters;
    float sampleRate = 44100.0;
    float frequency = 440.0;
    float frequencyL = 440.0;
    float frequencyR = 440.0;
    float phase = 0.0;
    float phaseL = 0.0;
    float phaseR = 0.0;
    float phaseIncrement = 0.0;
    float phaseIncrementL = 0.0;
    float phaseIncrementR = 0.0;
    float subPhase = 0.0;
    float subPhaseIncrement = 0.0;
    DynamicParameter& pulseWidth;
    float sawLevel = 0.0f;
    float pulseLevel = 1.0f;
    float subLevel = 0.0f;
    float detune = 0.0f;
    float detuneAmount = 0.25f;
    float stereoWidth = 1.0f;
    // size_t numSamples = 512;
    juce::dsp::Oversampling<float> oversampling = juce::dsp::Oversampling<float> (
        2, // Stereo
        2, // 2x oversampling
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
        true, // High-quality upsampling
        false // Normal-quality downsampling
    );
};