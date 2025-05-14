//
// Created by Tyler Teuber on 4/26/25.
//

#pragma once

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
          rate (0.5), // around 2.8 ms max
          depth (0.0028f),
          mix (0.5f),
          feedback (0.15f)
    {
        // Initialize LFO phases with stereo offset
        for (int channel = 0; channel < 2; ++channel)
        {
            // Create phase offsets for the stereo effect
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
            mix = newValue;
        }
        else if (parameterID == "chorusOn")
        {
            currentMode = newValue > 0.5f ? On : Off;
        }
        else if (parameterID == "chorusDepth")
        {
            depth = newValue;
        }
        else if (parameterID == "chorusRate")
        {
            rate = newValue;
            phaseIncrement = rate / sampleRate;
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

        // Clear any previous settings
        delayBuffer.setSize (2, 2 * static_cast<int> (sampleRate), false, true, false);
        delayBuffer.clear();

        // Calculate delay buffer length based on max possible delay
        delayBufferLength = static_cast<int> (2.0 * sampleRate);
        delayWritePosition = 0;

        // Initialize LFO
        phaseIncrement = rate / sampleRate;

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
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        // If the chorus is off, pass the signal through
        if (currentMode == Off)
            return;

        // Copy input to delay buffer
        for (int channel = 0; channel < juce::jmin (numChannels, 2); ++channel)
        {
            // Copy input to delay buffer
            if (delayBufferLength > 0)
            {
                const float* input = buffer.getReadPointer (channel);
                float* delayData = delayBuffer.getWritePointer (channel);

                for (int i = 0; i < numSamples; ++i)
                {
                    const int position = (delayWritePosition + i) % delayBufferLength;
                    delayData[position] = input[i] + feedback * delayData[position];
                }
            }
        }

        // Process with chorus effect
        for (int channel = 0; channel < juce::jmin (numChannels, 2); ++channel)
        {
            float* output = buffer.getWritePointer (channel);
            const float* delayData = delayBuffer.getReadPointer (channel);

            for (int i = 0; i < numSamples; ++i)
            {
                // Calculate the delay time using a channel-specific phase
                float delayTime = 0.0f;

                // Calculate LFO value with stereo width
                float lfoValue = 0.5f + 0.5f * sinf (2.0f * juce::MathConstants<float>::pi * phase[channel]);
                delayTime = depth * sampleRate * lfoValue;
                phase[channel] += phaseIncrement;
                if (phase[channel] >= 1.0f)
                    phase[channel] -= 1.0f;

                // Read the delayed sample
                int delayPos = static_cast<int> (delayWritePosition + i - delayTime + delayBufferLength) % delayBufferLength;

                // Ensure the position is valid
                jassert (delayPos >= 0 && delayPos < delayBufferLength);
                float wetSignal = filter[channel].processSingleSampleRaw (delayData[delayPos]);

                // Mix the original signal with the delayed signal
                const float drySignal = output[i];

                // Apply the wet/dry mix
                output[i] = (1.0f - mix) * drySignal + mix * wetSignal;
            }
        }

        // Update write position
        delayWritePosition = (delayWritePosition + numSamples) % delayBufferLength;
    }

    //==============================================================================
    // Set the chorus mode (OFF, ON)
    void setMode (ChorusMode mode)
    {
        currentMode = mode;
    }

    // Get the current mode
    ChorusMode getMode() const
    {
        return currentMode;
    }

    // Set the wet/dry mix amount (0.0 = dry only, 1.0 = wet only)
    void setMix (float newMix)
    {
        mix = juce::jlimit (0.0f, 1.0f, newMix);
    }

    // Get the current mix amount
    float getMix() const
    {
        return mix;
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
        rate = newRate;
        phaseIncrement = rate / sampleRate;
    }

    // Set the depth
    void setDepth (float newDepth)
    {
        depth = newDepth;
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

    // Parameters
    ChorusMode currentMode;

    // Delay line
    juce::AudioBuffer<float> delayBuffer;
    float sampleRate = 44100.0f;
    int delayBufferLength = 0;
    int delayWritePosition = 0;

    // LFO parameters
    float rate; // Hz
    float depth; // seconds
    float phase[2] = { 0, 0 }; // One phase per channel for stereo
    float phaseIncrement {};

    // Mixing and feedback
    float mix;
    float feedback;
    float stereoWidth = 1.0f; // Full stereo width

    // Filters for each channel
    juce::IIRFilter filter[2];

    //    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JuneChorus)
};