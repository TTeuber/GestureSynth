//
// Created by Tyler Teuber on 4/26/25.
//

#pragma once

#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

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
          currentMode (On),
          rate (0.1f),
          depth (0.001f),
          mix (0.5f),
          feedback (0.0f)
    {
        // Initialize LFO phases with stereo offset
        for (int channel = 0; channel < 2; ++channel)
        {
            phase[channel] = channel * 0.5f; // 180 degree offset between channels
        }

        parameters.addParameterListener ("chorusMix", this);
        parameters.addParameterListener ("chorusDepth", this);
        parameters.addParameterListener ("chorusRate", this);
        parameters.addParameterListener ("chorusOn", this);
    }

    ~JuneChorus() override
    {
        parameters.removeParameterListener ("chorusMix", this);
        parameters.removeParameterListener ("chorusOn", this);
        parameters.removeParameterListener ("chorusDepth", this);
        parameters.removeParameterListener ("chorusRate", this);
    }

    //==============================================================================

    void parameterChanged (const juce::String& parameterID, const float newValue) override
    {
        if (parameterID == "chorusMix")
        {
            mix.store (newValue, std::memory_order_relaxed);
        }
        else if (parameterID == "chorusOn")
        {
            currentMode.store (newValue > 0.5f ? On : Off, std::memory_order_relaxed);
        }
        else if (parameterID == "chorusDepth")
        {
            depth.store (newValue, std::memory_order_relaxed);
        }
        else if (parameterID == "chorusRate")
        {
            rate.store (newValue, std::memory_order_relaxed);
            phaseIncrement.store (newValue / sampleRate, std::memory_order_relaxed);
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
        phaseIncrement.store (rate.load (std::memory_order_relaxed) / sampleRate, std::memory_order_relaxed);

        // Initialize depth smoothing (20ms time constant)
        depthSmoothCoeff = 1.0f - std::exp (-1.0f / (0.02f * sampleRate));
        currentSmoothedDepth = depth.load (std::memory_order_relaxed);

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
            phase[i] = i * 0.5f;
        }

        delayWritePosition = 0;
        currentSmoothedDepth = depth.load (std::memory_order_relaxed);
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        if (delayBufferLength <= 0)
            return;

        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        // Load atomic parameters once per block
        const int mode = currentMode.load (std::memory_order_relaxed);
        if (mode == Off)
            return;

        const float localMix = mix.load (std::memory_order_relaxed);
        const float localDepth = depth.load (std::memory_order_relaxed);
        const float localPhaseInc = phaseIncrement.load (std::memory_order_relaxed);

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
        currentMode.store (mode, std::memory_order_relaxed);
    }

    // Get the current mode
    ChorusMode getMode() const
    {
        return static_cast<ChorusMode> (currentMode.load (std::memory_order_relaxed));
    }

    // Set the wet/dry mix amount (0.0 = dry only, 1.0 = wet only)
    void setMix (float newMix)
    {
        mix.store (juce::jlimit (0.0f, 1.0f, newMix), std::memory_order_relaxed);
    }

    // Get the current mix amount
    float getMix() const
    {
        return mix.load (std::memory_order_relaxed);
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
        rate.store (newRate, std::memory_order_relaxed);
        phaseIncrement.store (newRate / sampleRate, std::memory_order_relaxed);
    }

    // Set the depth
    void setDepth (float newDepth)
    {
        depth.store (newDepth, std::memory_order_relaxed);
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
