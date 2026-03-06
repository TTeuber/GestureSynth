#pragma once

#include "BBDDelay.h"
#include "Chorus.h"
#include "Reverb.h"
#include "../Utility/AtomicHelpers.h"
#include "../Utility/TempoInfo.h"
#include "../Utility/Parameters.h"
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
        // Read effects-level mod destinations written by voice rendering above.
        // These are "last-voice-wins" — all voices write to the same atomic slots,
        // so the final active voice's last sample value is what we read here.
        // This is correct because effects are global (post-voice-sum), not per-voice.
        {
            auto* depthParam = parameters.getParameter (ParamIDs::chorusDepth);
            auto* rateParam = parameters.getParameter (ParamIDs::chorusRate);
            chorus.setDepth (depthParam->convertFrom0to1 (AtomicHelpers::paramLoad (modDestOutputs[ModDestIndex::chorusDepth])));
            chorus.setRate (rateParam->convertFrom0to1 (AtomicHelpers::paramLoad (modDestOutputs[ModDestIndex::chorusRate])));
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
