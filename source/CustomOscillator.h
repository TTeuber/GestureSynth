#pragma once

#include <juce_dsp/juce_dsp.h>

class CustomOscillator
{
public:
    CustomOscillator();

    void setVolume (float newValue);
    void setFrequency (float newValue, bool force = false);
    void setFilterCutoff (float newCutoff);
    void setFilterResonance (float newResonance);

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
        gainIndex
    };

    juce::SmoothedValue<float> filterFrequencySmooth;

    juce::dsp::ProcessorChain<
        juce::dsp::Oscillator<float>,
        juce::dsp::Oscillator<float>,
        juce::dsp::LadderFilter<float>,
        juce::dsp::Gain<float>>
        processorChain;
};