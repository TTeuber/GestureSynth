//
// Created by Tyler Teuber on 4/26/25.
//

#pragma once

#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "../Utility/AtomicHelpers.h"
#include "../Utility/Parameters.h"

/**
 * JunoChorus - A class that mimics the classic Juno-6 chorus effect
 *
 * This implementation recreates the chorus effect with two states (OFF, ON).
 */
class JuneChorus : public juce::AudioProcessorValueTreeState::Listener
{
public:
    enum ChorusMode {
        Off = 0,
        On = 1
    };

    //==============================================================================
    explicit JuneChorus (juce::AudioProcessorValueTreeState& p)
        : parameters (p),
          currentMode (static_cast<int> (*p.getRawParameterValue (ParamIDs::chorusOn) > 0.5f ? On : Off)),
          rate (0.1f),
          depth (0.001f),
          mix (0.5f),
          feedback (0.0f)
    {
        // Initialize LFO phases with stereo offset
        for (int channel = 0; channel < 2; ++channel)
        {
            phase[channel] = static_cast<float> (channel) * 0.5f; // 180 degree offset between channels
        }

        parameters.addParameterListener (ParamIDs::chorusMix, this);
        parameters.addParameterListener (ParamIDs::chorusDepth, this);
        parameters.addParameterListener (ParamIDs::chorusRate, this);
        parameters.addParameterListener (ParamIDs::chorusOn, this);
    }

    ~JuneChorus() override
    {
        parameters.removeParameterListener (ParamIDs::chorusMix, this);
        parameters.removeParameterListener (ParamIDs::chorusOn, this);
        parameters.removeParameterListener (ParamIDs::chorusDepth, this);
        parameters.removeParameterListener (ParamIDs::chorusRate, this);
    }

    //==============================================================================

    void parameterChanged (const juce::String& parameterID, const float newValue) override
    {
        if (parameterID == ParamIDs::chorusMix)
        {
            AtomicHelpers::paramStore (mix, newValue);
        }
        else if (parameterID == ParamIDs::chorusOn)
        {
            AtomicHelpers::paramStore (currentMode, static_cast<int> (newValue > 0.5f ? On : Off));
        }
        else if (parameterID == ParamIDs::chorusDepth)
        {
            AtomicHelpers::paramStore (depth, newValue);
        }
        else if (parameterID == ParamIDs::chorusRate)
        {
            AtomicHelpers::paramStore (rate, newValue);
            AtomicHelpers::paramStore (phaseIncrement, newValue / sampleRate);
        }
        else
        {
            jassertfalse;
        }
    }

    //==============================================================================
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = static_cast<float> (spec.sampleRate);

        // 60ms buffer covers max depth (30ms) + base delay + headroom
        delayBufferLength = static_cast<int> (0.06f * sampleRate);
        delayBuffer.setSize (2, delayBufferLength, false, true, false);
        delayBuffer.clear();

        delayWritePosition = 0;

        // Initialize LFO
        AtomicHelpers::paramStore (phaseIncrement, AtomicHelpers::paramLoad (rate) / sampleRate);

        // Initialize depth smoothing (20ms time constant)
        depthSmoothCoeff = 1.0f - std::exp (-1.0f / (0.02f * sampleRate));
        currentSmoothedDepth = AtomicHelpers::paramLoad (depth);

        // Reset filters
        for (int i = 0; i < 2; ++i)
        {
            filter[i].reset();
            filter[i].setCoefficients (juce::IIRCoefficients::makeLowPass (sampleRate, 8000.0));
        }
    }

    void reset()
    {
        // Reset buffer and filter states
        delayBuffer.clear();
        for (int i = 0; i < 2; ++i)
        {
            filter[i].reset();

            // Reset phases but maintain stereo offset
            phase[i] = static_cast<float> (i) * 0.5f;
        }

        delayWritePosition = 0;
        currentSmoothedDepth = AtomicHelpers::paramLoad (depth);
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        if (delayBufferLength <= 0)
            return;

        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        // Load atomic parameters once per block
        const int mode = AtomicHelpers::paramLoad (currentMode);
        if (mode == Off)
            return;

        const float localMix = AtomicHelpers::paramLoad (mix);
        const float localDepth = AtomicHelpers::paramLoad (depth);
        const float localPhaseInc = AtomicHelpers::paramLoad (phaseIncrement);

        const int channelCount = juce::jmin (numChannels, 2);
        float* outputPtrs[2] = {};
        float* delayPtrs[2] = {};
        for (int ch = 0; ch < channelCount; ++ch)
        {
            outputPtrs[ch] = buffer.getWritePointer (ch);
            delayPtrs[ch] = delayBuffer.getWritePointer (ch);
        }

        constexpr float minDelaySeconds = 0.0005f; // 0.5ms base delay

        for (int i = 0; i < numSamples; ++i)
        {
            // Smooth depth once per sample to prevent pops/stepping
            currentSmoothedDepth += depthSmoothCoeff * (localDepth - currentSmoothedDepth);
            const int writePos = (delayWritePosition + i) % delayBufferLength;

            for (int channel = 0; channel < channelCount; ++channel)
            {
                const float drySignal = outputPtrs[channel][i];

                // Triangle LFO (0 to 1 range)
                const float lfoValue = 1.0f - std::abs (2.0f * phase[channel] - 1.0f);
                const float delayTime = (minDelaySeconds + currentSmoothedDepth * lfoValue) * sampleRate;

                phase[channel] += localPhaseInc;
                if (phase[channel] >= 1.0f)
                    phase[channel] -= 1.0f;

                // Read delayed sample with linear interpolation
                const float readPos = static_cast<float> (delayWritePosition + i) - delayTime
                                    + static_cast<float> (delayBufferLength);
                const int readIdx = static_cast<int> (readPos) % delayBufferLength;
                const int readIdxNext = (readIdx + 1) % delayBufferLength;
                const float frac = readPos - std::floor (readPos);

                const float interpolated = delayPtrs[channel][readIdx]
                                         + frac * (delayPtrs[channel][readIdxNext] - delayPtrs[channel][readIdx]);
                const float wetSignal = filter[channel].processSingleSampleRaw (interpolated);

                // Write input + feedback into delay buffer
                delayPtrs[channel][writePos] = drySignal + feedback * wetSignal;

                // Mix dry/wet
                outputPtrs[channel][i] = (1.0f - localMix) * drySignal + localMix * wetSignal;
            }
        }

        // Update write position
        delayWritePosition = (delayWritePosition + numSamples) % delayBufferLength;
    }

    //==============================================================================
    // Set the chorus mode (OFF, ON)
    void setMode (ChorusMode mode)
    {
        AtomicHelpers::paramStore (currentMode, static_cast<int> (mode));
    }

    // Get the current mode
    ChorusMode getMode() const
    {
        return static_cast<ChorusMode> (AtomicHelpers::paramLoad (currentMode));
    }

    // Set the wet/dry mix amount (0.0 = dry only, 1.0 = wet only)
    void setMix (float newMix)
    {
        AtomicHelpers::paramStore (mix, juce::jlimit (0.0f, 1.0f, newMix));
    }

    // Get the current mix amount
    float getMix() const
    {
        return AtomicHelpers::paramLoad (mix);
    }

    // Set the feedback amount
    void setFeedback (float newFeedback)
    {
        feedback = juce::jlimit (0.0f, 0.9f, newFeedback);
    }

    // Get the current feedback amount
    float getFeedback() const
    {
        return feedback;
    }

    // Set the rate
    void setRate (float newRate)
    {
        AtomicHelpers::paramStore (rate, newRate);
        AtomicHelpers::paramStore (phaseIncrement, newRate / sampleRate);
    }

    // Set the depth
    void setDepth (float newDepth)
    {
        AtomicHelpers::paramStore (depth, newDepth);
    }

    // Set the stereo width (0.0 = mono, 1.0 = full stereo)
    void setStereoWidth (float width)
    {
        stereoWidth = juce::jlimit (0.0f, 1.0f, width);
    }

    // Get the current stereo width
    float getStereoWidth() const
    {
        return stereoWidth;
    }

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState& parameters;

    // Parameters (atomic for thread safety between message and audio threads)
    std::atomic<int> currentMode;

    // Delay line
    juce::AudioBuffer<float> delayBuffer;
    float sampleRate = 44100.0f;
    int delayBufferLength = 0;
    int delayWritePosition = 0;

    // LFO parameters
    std::atomic<float> rate;
    std::atomic<float> depth;
    float phase[2] = { 0, 0 }; // One phase per channel for stereo
    std::atomic<float> phaseIncrement { 0.0f };

    // Mixing and feedback
    std::atomic<float> mix;
    float feedback;
    float stereoWidth = 1.0f; // Full stereo width

    // Depth smoothing
    float currentSmoothedDepth = 0.0028f;
    float depthSmoothCoeff = 0.0f;

    // Filters for each channel
    juce::IIRFilter filter[2];
};
