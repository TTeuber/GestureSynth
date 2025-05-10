//
// Created by Tyler Teuber on 4/26/25.
//

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

/**
 * JunoChorus - A class that mimics the classic Juno-6 chorus effect
 *
 * The Juno chorus had two modes (I and II) plus the ability to run both simultaneously.
 * This implementation recreates those three states (OFF, I, II, I+II).
 */
class JuneChorus : public juce::AudioProcessorValueTreeState::Listener
{
public:
    enum ChorusMode {
        Off = 0,
        Mode1 = 1,
        Mode2 = 2,
        ModeBoth = 3
    };

    //==============================================================================
    explicit JuneChorus (juce::AudioProcessorValueTreeState& p)
        : parameters (p),
          currentMode (Mode1),
          mode1Depth (0.0028f / 2.0f), // around 2.8 ms max for mode I
          mode2Depth (0.0018f / 2.0f), // around 1.8 ms max for mode II
          mix (0.5f),
          feedback (0.15f)
    {
        // Initialize LFO phases with stereo offset
        for (int channel = 0; channel < 2; ++channel)
        {
            // Create phase offsets for the stereo effect
            mode1Phase[channel] = channel * 0.5f; // 180 degree offset between channels
            mode2Phase[channel] = channel * 0.25f; // 90 degree offset between channels
        }

        parameters.addParameterListener ("chorusMix", this);
    }

    ~JuneChorus() override
    {
        parameters.removeParameterListener ("chorusMix", this);
    }

    //==============================================================================

    void parameterChanged (const juce::String& parameterID, const float newValue) override
    {
        if (parameterID == "chorusMix")
        {
            mix = newValue;
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

        // Initialize LFOs
        mode1PhaseIncrement = mode1Rate / sampleRate;
        mode2PhaseIncrement = mode2Rate / sampleRate;

        // Reset filters
        for (int i = 0; i < 2; ++i)
        {
            mode1Filter[i].reset();
            mode2Filter[i].reset();
            mode1Filter[i].setCoefficients (juce::IIRCoefficients::makeLowPass (sampleRate, 8000.0));
            mode2Filter[i].setCoefficients (juce::IIRCoefficients::makeLowPass (sampleRate, 8000.0));
        }
    }

    void reset()
    {
        // Reset buffer and filter states
        delayBuffer.clear();
        for (int i = 0; i < 2; ++i)
        {
            mode1Filter[i].reset();
            mode2Filter[i].reset();

            // Reset phases but maintain stereo offset
            mode1Phase[i] = i * 0.5f;
            mode2Phase[i] = i * 0.25f;
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
                // Calculate the delay time for each mode using a channel-specific phase
                float mode1Delay = 0.0f;
                float mode2Delay = 0.0f;

                if (currentMode == Mode1 || currentMode == ModeBoth)
                {
                    // Calculate LFO value for mode 1 with stereo width
                    float lfoValue = 0.5f + 0.5f * sinf (2.0f * juce::MathConstants<float>::pi * mode1Phase[channel]);
                    mode1Delay = mode1Depth * sampleRate * lfoValue;
                    mode1Phase[channel] += mode1PhaseIncrement;
                    if (mode1Phase[channel] >= 1.0f)
                        mode1Phase[channel] -= 1.0f;
                }

                if (currentMode == Mode2 || currentMode == ModeBoth)
                {
                    // Calculate LFO value for mode 2 with stereo width
                    float lfoValue = 0.5f + 0.5f * sinf (2.0f * juce::MathConstants<float>::pi * mode2Phase[channel]);
                    mode2Delay = mode2Depth * sampleRate * lfoValue;
                    mode2Phase[channel] += mode2PhaseIncrement;
                    if (mode2Phase[channel] >= 1.0f)
                        mode2Phase[channel] -= 1.0f;
                }

                // Read the delayed samples
                float outMode1 = 0.0f;
                float outMode2 = 0.0f;

                if (currentMode == Mode1 || currentMode == ModeBoth)
                {
                    int delayPos1 = static_cast<int> (delayWritePosition + i - mode1Delay + delayBufferLength) % delayBufferLength;
                    // Ensure the position is valid
                    jassert (delayPos1 >= 0 && delayPos1 < delayBufferLength);
                    outMode1 = mode1Filter[channel].processSingleSampleRaw (delayData[delayPos1]);
                }

                if (currentMode == Mode2 || currentMode == ModeBoth)
                {
                    int delayPos2 = static_cast<int> (delayWritePosition + i - mode2Delay + delayBufferLength) % delayBufferLength;
                    // Ensure the position is valid
                    jassert (delayPos2 >= 0 && delayPos2 < delayBufferLength);
                    outMode2 = mode2Filter[channel].processSingleSampleRaw (delayData[delayPos2]);
                }

                // Mix the original signal with the delayed signals
                const float drySignal = output[i];
                float wetSignal = 0.0f;

                switch (currentMode)
                {
                    case Mode1:
                        wetSignal = outMode1;
                        break;
                    case Mode2:
                        wetSignal = outMode2;
                        break;
                    case ModeBoth:
                        wetSignal = 0.5f * (outMode1 + outMode2);
                        break;
                    default:
                        break;
                }

                // Apply the wet/dry mix
                output[i] = (1.0f - mix) * drySignal + mix * wetSignal;
            }
        }

        // Update write position
        delayWritePosition = (delayWritePosition + numSamples) % delayBufferLength;
    }

    //==============================================================================
    // Set the chorus mode (OFF, I, II, I+II)
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

    // Set the rate for Mode I
    void setMode1Rate (float newRate)
    {
        mode1Rate = newRate;
        mode1PhaseIncrement = mode1Rate / sampleRate;
    }

    // Set the rate for Mode II
    void setMode2Rate (float newRate)
    {
        mode2Rate = newRate;
        mode2PhaseIncrement = mode2Rate / sampleRate;
    }

    // Set the depth for Mode I
    void setMode1Depth (float newDepth)
    {
        mode1Depth = newDepth;
    }

    // Set the depth for Mode II
    void setMode2Depth (float newDepth)
    {
        mode2Depth = newDepth;
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
    float mode1Rate = 0.5; // Hz
    float mode2Rate = 0.8; // Hz
    float mode1Depth; // seconds
    float mode2Depth; // seconds
    float mode1Phase[2] = { 0, 0 }; // One phase per channel for stereo
    float mode2Phase[2] = { 0, 0 }; // One phase per channel for stereo
    float mode1PhaseIncrement {};
    float mode2PhaseIncrement {};

    // Mixing and feedback
    float mix;
    float feedback;
    float stereoWidth = 1.0f; // Full stereo width

    // Filters for each channel and mode
    juce::IIRFilter mode1Filter[2];
    juce::IIRFilter mode2Filter[2];

    //    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JuneChorus)
};