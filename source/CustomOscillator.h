#pragma once

#include <juce_dsp/juce_dsp.h>

class CustomOscillator
{
public:
    CustomOscillator()
    {
        auto& osc = processorChain.template get<osc1Index>();
        osc.initialise ([] (float x) {
            return juce::jmap<float> (x,
                float (-juce::MathConstants<double>::pi),
                float (juce::MathConstants<double>::pi),
                float (-1),
                float (1));
        },
            2);

        auto& osc2 = processorChain.template get<osc2Index>();
        osc2.initialise ([] (float x) {
            return juce::jmap<float> (x,
                float (-juce::MathConstants<double>::pi),
                float (juce::MathConstants<double>::pi),
                float (-1),
                float (1));
        },
            2);

        auto& filter = processorChain.template get<filterIndex>();
        filter.setCutoffFrequencyHz (2000.0f);
        filter.setResonance (0.0f);
    }
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        processorChain.prepare (spec);
    }
    void reset() noexcept
    {
        processorChain.reset();
    }
    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        processorChain.process (context); // [9]
    }

    void setFrequency (float newValue, bool force = false)
    {
        float detuneAmount = (juce::Random::getSystemRandom().nextFloat() - 0.5f) * 0.2f; // ±0.1 semitone
        auto& osc1 = processorChain.template get<osc1Index>();
        auto& osc2 = processorChain.template get<osc2Index>();
        osc1.setFrequency (newValue * std::pow (2.0f, detuneAmount / 12.0f), force);
        osc2.setFrequency (newValue * 1.01f * std::pow (2.0f, detuneAmount / 12.0f), force);
    }

    float getVolume() { return processorChain.template get<gainIndex>().getGainLinear(); }
    void setVolume (float newValue)
    {
        auto& gain = processorChain.get<gainIndex>();
        gain.setGainLinear (newValue);
    }

    void setFilterCutoff (float newCutoff)
    {
        filterFrequencySmooth.setTargetValue (newCutoff);
        auto& filter = processorChain.template get<filterIndex>();
        filter.setCutoffFrequencyHz (filterFrequencySmooth.getNextValue());
    }

    void setFilterResonance (float newResonance)
    {
        auto& filter = processorChain.template get<filterIndex>();
        filter.setResonance (newResonance);
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