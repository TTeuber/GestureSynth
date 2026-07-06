//
// Created by Tyler Teuber on 4/25/25.
//

#pragma once
#include "../Utility/MyParameter.h"
#include "../Utility/Parameters.h"
#include <juce_dsp/juce_dsp.h>

class JuneDCO final : public juce::AudioProcessorValueTreeState::Listener
{
public:
    JuneDCO (juce::AudioProcessorValueTreeState& p, DynamicParameter& pw, DynamicParameter& wf,
        DynamicParameter& dt, DynamicParameter& ow, DynamicParameter& so, DynamicParameter& sw,
        DynamicParameter& mol)
        : parameters (p), pulseWidth (pw), waveformMix (wf),
          detuneParam (dt), widthParam (ow), subOscParam (so), subOscWaveParam (sw),
          mainOscLevelParam (mol)
    {
        oscillatorEnabled = *p.getRawParameterValue (ParamIDs::oscOn) > 0.5f;
        subOscillatorEnabled = *p.getRawParameterValue (ParamIDs::subOn) > 0.5f;

        parameters.addParameterListener (ParamIDs::oscOn, this);
        parameters.addParameterListener (ParamIDs::subOn, this);

        reset();
    }
    ~JuneDCO() override
    {
        parameters.removeParameterListener (ParamIDs::oscOn, this);
        parameters.removeParameterListener (ParamIDs::subOn, this);
    }

    void parameterChanged (const juce::String& parameterID, float newValue) override
    {
        if (parameterID == ParamIDs::oscOn)
        {
            oscillatorEnabled = newValue > 0.5f;
        }
        else if (parameterID == ParamIDs::subOn)
        {
            subOscillatorEnabled = newValue > 0.5f;
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
        const float currentDetune = detuneParam.getCurrentValue();
        frequencyL = frequency * std::pow (2.0f, currentDetune * detuneAmount / 12.0f);
        frequencyR = frequency * std::pow (2.0f, -(currentDetune * detuneAmount) / 12.0f);
        const float oversampledSampleRate = sampleRate * static_cast<float> (oversampling.getOversamplingFactor());
        phaseIncrement = frequency / oversampledSampleRate;
        phaseIncrementL = frequencyL / oversampledSampleRate;
        phaseIncrementR = frequencyR / oversampledSampleRate;
        subPhaseIncrement = frequency / oversampledSampleRate / 2; // Sub-oscillator is one octave lower
    }

    // PolyBLEP function to reduce aliasing.
    // Call unconditionally with the (wrapped) distance from the discontinuity:
    // both the sample just after (t < dt) and just before (t > 1 - dt) an edge
    // need correcting, so gating the call on "did we just wrap" drops half the
    // correction and leaves ~15 dB of alias suppression on the table.
    // Subtract for falling edges, add for rising edges.
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

    // Wraps a phase offset (e.g. distance from the pulse-width edge) into [0, 1)
    static float wrapPhase (const float t)
    {
        return t < 0.0f ? t + 1.0f : t;
    }

    // Process a single sample
    std::array<float, 2> process()
    {
        // Update phase (0.0 to 1.0)
        phase += phaseIncrement;
        phaseL += phaseIncrementL;
        phaseR += phaseIncrementR;
        if (phase >= 1.0)
            phase -= 1.0f;
        if (phaseL >= 1.0)
            phaseL -= 1.0f;
        if (phaseR >= 1.0)
            phaseR -= 1.0f;

        // Update sub-oscillator phase
        subPhase += subPhaseIncrement;
        if (subPhase >= 1.0)
            subPhase -= 1.0f;

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
            // The saw ramps up and falls at the wrap: subtract the polyBLEP.
            // The pulse here is -1 below the width point and +1 above, so its
            // edges are: falling at the wrap, rising at the width point.
            const float pw = pulseWidth.getCurrentValue();
            if (detuneEnabled)
            {
                // Detuned stereo processing
                sawOutputL = 2.0f * phaseL - 1.0f - polyBlep (phaseL, phaseIncrementL);
                sawOutputR = 2.0f * phaseR - 1.0f - polyBlep (phaseR, phaseIncrementR);

                // Pulse wave processing
                pulseOutputL = (phaseL < pw) ? -1.0f : 1.0f;
                pulseOutputL -= polyBlep (phaseL, phaseIncrementL);
                pulseOutputL += polyBlep (wrapPhase (phaseL - pw), phaseIncrementL);

                pulseOutputR = (phaseR < pw) ? -1.0f : 1.0f;
                pulseOutputR -= polyBlep (phaseR, phaseIncrementR);
                pulseOutputR += polyBlep (wrapPhase (phaseR - pw), phaseIncrementR);
            }
            else
            {
                // Mono processing
                sawOutput = 2.0f * phase - 1.0f - polyBlep (phase, phaseIncrement);

                // Pulse wave processing
                pulseOutput = (phase < pw) ? -1.0f : 1.0f;
                pulseOutput -= polyBlep (phase, phaseIncrement);
                pulseOutput += polyBlep (wrapPhase (phase - pw), phaseIncrement);
            }
        }

        // Process sub oscillator if enabled
        if (subOscillatorEnabled)
        {
            // Waveform generators with PolyBLEP where needed
            auto genSine = [&]()
            {
                return std::sin (2.0f * juce::MathConstants<float>::pi * subPhase);
            };

            auto genTriangle = [&]()
            {
                return 2.0f * std::abs (2.0f * subPhase - 1.0f) - 1.0f;
            };

            auto genSquare = [&]()
            {
                // +1 below the midpoint: rising edge at the wrap, falling at 0.5
                float sq = (subPhase < 0.5f) ? 1.0f : -1.0f;
                sq += polyBlep (subPhase, subPhaseIncrement);
                sq -= polyBlep (wrapPhase (subPhase - 0.5f), subPhaseIncrement);
                return sq;
            };

            auto genSaw = [&]()
            {
                // Down ramp: 1 - 2*phase, rising edge at the wrap
                return 1.0f - 2.0f * subPhase + polyBlep (subPhase, subPhaseIncrement);
            };

            // Segment mapping: 0.0=Sine, 1/3=Triangle, 2/3=Square, 1.0=Saw
            const float scaled = subOscWaveParam.getCurrentValue() * 3.0f;
            const int seg = juce::jmin (2, static_cast<int> (scaled));
            const float blend = scaled - static_cast<float> (seg);

            float waveA, waveB;
            switch (seg)
            {
                case 0: waveA = genSine(); waveB = genTriangle(); break;
                case 1: waveA = genTriangle(); waveB = genSquare(); break;
                case 2: waveA = genSquare(); waveB = genSaw(); break;
                default: waveA = genSine(); waveB = genSine(); break;
            }
            subOutput = waveA + blend * (waveB - waveA);
        }

        // Mix waveforms
        const float currentWaveformMix = waveformMix.getCurrentValue();
        const float liveSawLevel = 1.0f - currentWaveformMix;
        const float livePulseLevel = currentWaveformMix;
        float outputL, outputR;

        const float currentSubLevel = subOscParam.getCurrentValue();
        const float currentMainOscLevel = mainOscLevelParam.getCurrentValue();
        const float currentStereoWidth = widthParam.getCurrentValue();

        if (detuneEnabled)
        {
            // Mix stereo signals
            float leftSignal = (sawOutputL * liveSawLevel + pulseOutputL * livePulseLevel) * currentMainOscLevel + subOutput * currentSubLevel;
            float rightSignal = (sawOutputR * liveSawLevel + pulseOutputR * livePulseLevel) * currentMainOscLevel + subOutput * currentSubLevel;

            // Apply stereo width
            // When stereoWidth is 0, both channels should be the same (mono)
            // When stereoWidth is 1, channels are fully separated
            float monoSignal = (leftSignal + rightSignal) * 0.5f;

            outputL = monoSignal * (1.0f - currentStereoWidth) + leftSignal * currentStereoWidth;
            outputR = monoSignal * (1.0f - currentStereoWidth) + rightSignal * currentStereoWidth;
        }
        else
        {
            // Mix mono signal
            float monoOut = (sawOutput * liveSawLevel + pulseOutput * livePulseLevel) * currentMainOscLevel + subOutput * currentSubLevel;
            outputL = monoOut;
            outputR = monoOut;
        }

        // Normalize output to prevent clipping
        float activeMixSum = 0.0f;
        if (oscillatorEnabled)
            activeMixSum += (liveSawLevel + livePulseLevel) * currentMainOscLevel;
        if (subOscillatorEnabled)
            activeMixSum += currentSubLevel;

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

        for (int i = 0; i < static_cast<int> (osBuffer.getNumSamples()); ++i)
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
    DynamicParameter& detuneParam;
    DynamicParameter& widthParam;
    DynamicParameter& subOscParam;
    DynamicParameter& subOscWaveParam;
    DynamicParameter& mainOscLevelParam;
    float detuneAmount = 0.25f;
    bool oscillatorEnabled = false;
    bool subOscillatorEnabled = false;
    static constexpr bool detuneEnabled = true;
    // size_t numSamples = 512;
    juce::dsp::Oversampling<float> oversampling = juce::dsp::Oversampling<float> (
        2,     // numChannels (stereo)
        2,     // order: 2^2 = 4x oversampling
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,  // isMaxQuality (high-quality filtering)
        true   // useIntegerLatency (needed for host latency reporting)
    );
};