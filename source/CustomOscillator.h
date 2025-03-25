#pragma once

#include "MyOscillator.h"

#include <juce_dsp/juce_dsp.h>

class CustomOscillator
{
public:
    CustomOscillator();

    void setVolume (float newValue);
    void setFrequency (float newValue, bool force = false);
    void setFilterCutoff (float newCutoff);
    void setFilterResonance (float newResonance);

    void setChorusRate (float newRate) { processorChain.get<chorusIndex>().setRate (newRate); }
    void setChorusDepth (float newDepth) { processorChain.get<chorusIndex>().setDepth (newDepth); }

    void prepare (const juce::dsp::ProcessSpec& spec) { processorChain.prepare (spec); }
    void reset() noexcept { processorChain.reset(); }
    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        processorChain.process (context);
    }

private:
    enum {
        osc1Index,
        osc2Index,
        filterIndex,
        chorusIndex,
        gainIndex
    };

    juce::SmoothedValue<float> filterFrequencySmooth;

    juce::dsp::ProcessorChain<
        MyOscillator,
        MyOscillator,
        juce::dsp::LadderFilter<float>,
        juce::dsp::Chorus<float>,
        juce::dsp::Gain<float>>
        processorChain;
};