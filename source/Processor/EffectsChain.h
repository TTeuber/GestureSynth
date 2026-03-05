#pragma once

#include "BBDDelay.h"
#include "Chorus.h"
#include "Reverb.h"
#include "../Utility/TempoInfo.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

class EffectsChain
{
public:
    explicit EffectsChain (juce::AudioProcessorValueTreeState& p)
        : parameters (p),
          chorus (p),
          bbdDelay (p),
          reverb (p)
    {
    }

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        chorus.prepare (spec);
        bbdDelay.prepare (spec);
        reverb.prepare (spec);
    }

    template <size_t N>
    void process (juce::AudioBuffer<float>& buffer,
                  const TempoInfo& tempoInfo,
                  std::array<std::atomic<float>, N>& modDestOutputs)
    {
        // Apply modulated chorus depth/rate from mod matrix (last-voice-wins)
        // modDestOutputs stores normalized 0-1 values; convert to actual parameter range
        {
            auto* depthParam = parameters.getParameter ("chorusDepth");
            auto* rateParam = parameters.getParameter ("chorusRate");
            chorus.setDepth (depthParam->convertFrom0to1 (modDestOutputs[12].load (std::memory_order_relaxed)));
            chorus.setRate (rateParam->convertFrom0to1 (modDestOutputs[13].load (std::memory_order_relaxed)));
        }

        chorus.process (buffer);

        bbdDelay.setTempoInfo (tempoInfo.bpm, tempoInfo.hostTempoAvailable);
        bbdDelay.process (buffer);

        reverb.process (buffer);

        softClip (buffer);
    }

private:
    juce::AudioProcessorValueTreeState& parameters;
    JuneChorus chorus;
    BBDDelay bbdDelay;
    Reverb reverb;

    // Soft-clip the output to tame peaks from polyphonic voice summing
    // Uses Pade approximant of tanh: x*(27+x^2)/(27+9x^2), accurate to ~1% for |x|<3
    static void softClip (juce::AudioBuffer<float>& buffer)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* samples = buffer.getWritePointer (ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const float x = samples[i];
                const float x2 = x * x;
                samples[i] = juce::jlimit (-1.0f, 1.0f, x * (27.0f + x2) / (27.0f + 9.0f * x2));
            }
        }
    }
};
