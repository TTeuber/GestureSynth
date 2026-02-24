//
// Created by Tyler Teuber on 4/25/25.
//

#pragma once
#include "../Utility/MyParameter.h"
#include <juce_dsp/juce_dsp.h>

class JuneDCO final : public juce::AudioProcessorValueTreeState::Listener
{
public:
    // Define enum for waveform types
    enum class SubWaveType {
        Sine = 1,
        Triangle = 2,
        Square = 3,
        Saw = 4
    };

    JuneDCO (juce::AudioProcessorValueTreeState& p, DynamicParameter& pw, DynamicParameter& wf)
        : parameters (p), pulseWidth (pw), waveformMix (wf)
    {
        parameters.addParameterListener ("oscDetune", this);
        parameters.addParameterListener ("oscWidth", this);
        parameters.addParameterListener ("subOsc", this);
        parameters.addParameterListener ("subOscWave", this);
        parameters.addParameterListener ("oscOn", this);
        parameters.addParameterListener ("subOn", this);
        parameters.addParameterListener ("detuneOn", this);

        reset();
    }
    ~JuneDCO() override
    {
        parameters.removeParameterListener ("oscDetune", this);
        parameters.removeParameterListener ("oscWidth", this);
        parameters.removeParameterListener ("subOsc", this);
        parameters.removeParameterListener ("subOscWave", this);
        parameters.removeParameterListener ("oscOn", this);
        parameters.removeParameterListener ("subOn", this);
        parameters.removeParameterListener ("detuneOn", this);
    }

    void parameterChanged (const juce::String& parameterID, float newValue) override
    {
        if (parameterID == "oscDetune")
        {
            // Handle detune change if needed
            detune = newValue;
            frequencyL = frequency * std::pow (2, detune * detuneAmount / 12.0f);
            frequencyR = frequency * std::pow (2, -(detune * detuneAmount) / 12.0f);
            const float oversampledSampleRate = sampleRate * static_cast<float> (oversampling.getOversamplingFactor());
            phaseIncrementL = frequencyL / oversampledSampleRate;
            phaseIncrementR = frequencyR / oversampledSampleRate;
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
        else if (parameterID == "subOscWave")
        {
            // Handle sub-oscillator waveform change
            int waveformValue = static_cast<int> (newValue);
            subOscWaveform = static_cast<SubWaveType> (waveformValue);
        }
        else if (parameterID == "oscOn")
        {
            oscillatorEnabled = newValue > 0.5f;
        }
        else if (parameterID == "subOn")
        {
            subOscillatorEnabled = newValue > 0.5f;
        }
        else if (parameterID == "detuneOn")
        {
            detuneEnabled = newValue > 0.5f;
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
        phase = 0.0;
        phaseL = 0.0;
        phaseR = 0.0;
        subPhase = 0.0;
    }

    // Set frequency in Hz
    void setFrequency (const float freq)
    {
        frequency = freq;
        frequencyL = frequency * std::pow (2, detune * detuneAmount / 12.0f);
        frequencyR = frequency * std::pow (2, -(detune * detuneAmount) / 12.0f);
        const float oversampledSampleRate = sampleRate * static_cast<float> (oversampling.getOversamplingFactor());
        phaseIncrement = frequency / oversampledSampleRate;
        phaseIncrementL = frequencyL / oversampledSampleRate;
        phaseIncrementR = frequencyR / oversampledSampleRate;
        subPhaseIncrement = frequency / oversampledSampleRate / 2; // Sub-oscillator is one octave lower
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
        float previousPhase = phase;
        float previousPhaseL = phaseL;
        float previousPhaseR = phaseR;
        float previousSubPhase = subPhase;

        // Update phase (0.0 to 1.0)
        phase += phaseIncrement;
        phaseL += phaseIncrementL;
        phaseR += phaseIncrementR;
        if (phase >= 1.0)
            phase -= 1.0;
        if (phaseL >= 1.0)
            phaseL -= 1.0;
        if (phaseR >= 1.0)
            phaseR -= 1.0;

        // Update sub-oscillator phase
        subPhase += subPhaseIncrement;
        if (subPhase >= 1.0)
            subPhase -= 1.0;

        // Initialize outputs
        float sawOutput = 0.0f;
        float sawOutputL = 0.0f;
        float sawOutputR = 0.0f;
        float pulseOutput = 0.0f;
        float pulseOutputL = 0.0f;
        float pulseOutputR = 0.0f;
        float subOutput = 0.0f;

        // Process oscillators based on enabled flags
        if (oscillatorEnabled)
        {
            // Process main oscillators
            if (detuneEnabled)
            {
                // Detuned stereo processing
                sawOutputL = 2.0f * phaseL - 1.0f;
                sawOutputR = 2.0f * phaseR - 1.0f;

                // Apply PolyBLEP correction at discontinuity
                if (previousPhaseL > phaseL)
                    sawOutputL -= polyBlep (phaseL, phaseIncrementL);
                if (previousPhaseR > phaseR)
                    sawOutputR -= polyBlep (phaseR, phaseIncrementR);

                // Pulse wave processing
                pulseOutputL = (phaseL < pulseWidth.getCurrentValue()) ? -1.0 : 1.0;
                pulseOutputR = (phaseR < pulseWidth.getCurrentValue()) ? -1.0 : 1.0;

                // Apply PolyBLEP correction at rising and falling edges
                if (phaseL < phaseIncrementL)
                    pulseOutputL += polyBlep (phaseL, phaseIncrementL);
                if (phaseL > pulseWidth.getCurrentValue() && phaseL < pulseWidth.getCurrentValue() + phaseIncrementL)
                    pulseOutputL -= polyBlep (phaseL - pulseWidth.getCurrentValue(), phaseIncrementL);
                if (phaseR < phaseIncrementR)
                    pulseOutputR += polyBlep (phaseR, phaseIncrementR);
                if (phaseR > pulseWidth.getCurrentValue() && phaseR < pulseWidth.getCurrentValue() + phaseIncrementR)
                    pulseOutputR -= polyBlep (phaseR - pulseWidth.getCurrentValue(), phaseIncrementR);
            }
            else
            {
                // Mono processing
                sawOutput = 2.0f * phase - 1.0f;

                // Apply PolyBLEP correction at discontinuity
                if (previousPhase > phase)
                    sawOutput -= polyBlep (phase, phaseIncrement);

                // Pulse wave processing
                pulseOutput = (phase < pulseWidth.getCurrentValue()) ? -1.0 : 1.0;

                // Apply PolyBLEP correction at rising and falling edges
                if (phase < phaseIncrement)
                    pulseOutput += polyBlep (phase, phaseIncrement);
                if (phase > pulseWidth.getCurrentValue() && phase < pulseWidth.getCurrentValue() + phaseIncrement)
                    pulseOutput -= polyBlep (phase - pulseWidth.getCurrentValue(), phaseIncrement);
            }
        }

        // Process sub oscillator if enabled
        if (subOscillatorEnabled)
        {
            // Generate different waveforms based on subOscWaveform value
            switch (subOscWaveform)
            {
                case SubWaveType::Sine:
                    // Sine wave: sin(2*PI*phase)
                    subOutput = std::sin (2.0f * juce::MathConstants<float>::pi * subPhase);
                    break;

                case SubWaveType::Triangle:
                    // Triangle wave: 2.0 * |2.0 * phase - 1.0| - 1.0
                    subOutput = 2.0f * std::abs (2.0f * subPhase - 1.0f) - 1.0f;
                    break;

                case SubWaveType::Square:
                    // Square wave (original implementation)
                    subOutput = (subPhase < 0.5) ? 1.0 : -1.0;

                    // Apply PolyBLEP correction for sub-oscillator
                    if (subPhase < subPhaseIncrement)
                        subOutput += polyBlep (subPhase, subPhaseIncrement);
                    if (subPhase > 0.5 && subPhase < 0.5 + subPhaseIncrement)
                        subOutput -= polyBlep (subPhase - 0.5, subPhaseIncrement);
                    break;

                case SubWaveType::Saw:
                    // Sawtooth wave: 2.0 * phase - 1.0
                    subOutput = 2.0f * subPhase - 1.0f;

                    // Apply PolyBLEP correction at discontinuity
                    if (previousSubPhase > subPhase)
                        subOutput -= polyBlep (subPhase, subPhaseIncrement);
                    break;
            }
        }

        // Mix waveforms
        const float currentWaveformMix = waveformMix.getCurrentValue();
        const float liveSawLevel = 1.0f - currentWaveformMix;
        const float livePulseLevel = currentWaveformMix;
        float outputL, outputR;

        if (detuneEnabled)
        {
            // Mix stereo signals
            float leftSignal = sawOutputL * liveSawLevel + pulseOutputL * livePulseLevel + subOutput * subLevel;
            float rightSignal = sawOutputR * liveSawLevel + pulseOutputR * livePulseLevel + subOutput * subLevel;

            // Apply stereo width
            // When stereoWidth is 0, both channels should be the same (mono)
            // When stereoWidth is 1, channels are fully separated
            float monoSignal = (leftSignal + rightSignal) * 0.5f;

            outputL = monoSignal * (1.0f - stereoWidth) + leftSignal * stereoWidth;
            outputR = monoSignal * (1.0f - stereoWidth) + rightSignal * stereoWidth;
        }
        else
        {
            // Mix mono signal
            float monoOut = sawOutput * liveSawLevel + pulseOutput * livePulseLevel + subOutput * subLevel;
            outputL = monoOut;
            outputR = monoOut;
        }

        // Normalize output to prevent clipping
        float activeMixSum = 0.0f;
        if (oscillatorEnabled)
            activeMixSum += liveSawLevel + livePulseLevel;
        if (subOscillatorEnabled)
            activeMixSum += subLevel;

        if (activeMixSum > 0.0f)
        {
            outputL /= activeMixSum;
            outputR /= activeMixSum;
        }

        return { outputL, outputR };
    }

    // Process a block of samples
    void processBlock (juce::dsp::AudioBlock<float>& buffer)
    {
        if (!oscillatorEnabled && !subOscillatorEnabled)
            return;

        const auto osBuffer = oversampling.processSamplesUp (buffer);

        for (int i = 0; i < osBuffer.getNumSamples(); ++i)
        {
            const std::array<float, 2> vals = process();
            osBuffer.getChannelPointer (0)[i] = vals[0];
            osBuffer.getChannelPointer (1)[i] = vals[1];
        }

        oversampling.processSamplesDown (buffer);
    }

    float getLatencyInSamples() const { return oversampling.getLatencyInSamples(); }

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
    DynamicParameter& waveformMix;
    float subLevel = 0.0f;
    float detune = 0.0f;
    float detuneAmount = 0.25f;
    float stereoWidth = 1.0f;
    bool oscillatorEnabled = true; // Default to on
    bool subOscillatorEnabled = true; // Default to on
    bool detuneEnabled = true; // Default to on
    SubWaveType subOscWaveform = SubWaveType::Square; // Default to square wave
    // size_t numSamples = 512;
    juce::dsp::Oversampling<float> oversampling = juce::dsp::Oversampling<float> (
        2,     // numChannels (stereo)
        2,     // order: 2^2 = 4x oversampling
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,  // isMaxQuality (high-quality filtering)
        true   // useIntegerLatency (needed for host latency reporting)
    );
};