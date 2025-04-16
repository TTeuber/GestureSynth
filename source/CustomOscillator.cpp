//
// Created by Tyler Teuber on 2/24/25.
//

#include "CustomOscillator.h"

CustomOscillator::CustomOscillator()
{
    auto& osc = processorChain.get<osc1Index>();
    osc.setWaveform (MyOscillator::Saw);

    // auto& osc2 = processorChain.get<osc2Index>();
    // osc2.setWaveform (MyOscillator::Saw);

    auto& filter = processorChain.get<filterIndex>();
    filter.setCutoffFrequencyHz (2000.0f);
    filter.setResonance (0.0f);

    // auto& chorus = processorChain.get<chorusIndex>();
    // chorus.setRate (0.5f);
    // chorus.setDepth (0.5f);
    // chorus.setFeedback (0.3f);
    // chorus.setCentreDelay (7.0f);
    // chorus.setMix (0.5f);
}
void CustomOscillator::setFrequency (const float newValue, const bool force)
{
    const float detuneAmount = (juce::Random::getSystemRandom().nextFloat() - 0.5f) * 0.2f; // ±0.1 semitone
    auto& osc1 = processorChain.get<osc1Index>();
    // auto& osc2 = processorChain.get<osc2Index>();
    osc1.setFrequency (newValue * std::pow (2.0f, detuneAmount / 12.0f), force);
    // osc2.setFrequency (newValue * 1.01f * std::pow (2.0f, detuneAmount / 12.0f), force);
}
void CustomOscillator::setVolume (const float newValue)
{
    auto& gain = processorChain.get<gainIndex>();
    gain.setGainLinear (newValue);
}

void CustomOscillator::setFilterCutoff (const float newCutoff)
{
    filterFrequencySmooth.setTargetValue (newCutoff);
    auto& filter = processorChain.get<filterIndex>();
    filter.setCutoffFrequencyHz (filterFrequencySmooth.getNextValue());
}

void CustomOscillator::setFilterResonance (const float newResonance)
{
    auto& filter = processorChain.get<filterIndex>();
    filter.setResonance (newResonance);
}
