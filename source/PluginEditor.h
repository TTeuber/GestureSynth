#pragma once

#include "ParameterDial.h"
#include "PluginProcessor.h"
#include <juce_audio_utils/juce_audio_utils.h>

//==============================================================================
class PluginEditor : public juce::AudioProcessorEditor, public juce::MidiKeyboardStateListener
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    PluginProcessor& processorRef;

    juce::MidiKeyboardComponent keyboardComponent;

    ParameterDial volumeDial = { processorRef, "volume", "Volume" };
    ParameterDial ampAttackDial = { processorRef, "ampAttack", "Amp Attack" };
    ParameterDial ampDecayDial = { processorRef, "ampDecay", "Amp Decay" };
    ParameterDial ampSustainDial = { processorRef, "ampSustain", "Amp Sustain" };
    ParameterDial ampReleaseDial = { processorRef, "ampRelease", "Amp Release" };
    ParameterDial filterFrequencyDial = { processorRef, "filterFrequency", "Filter Frequency" };
    ParameterDial filterEnvelopeAmountDial = { processorRef, "filterEnvelopeAmount", "Filter Envelope Amount" };
    ParameterDial filterResonanceDial = { processorRef, "filterResonance", "Filter Resonance" };
    ParameterDial filterAttackDial = { processorRef, "filterAttack", "Filter Attack" };
    ParameterDial filterDecayDial = { processorRef, "filterDecay", "Filter Decay" };
    ParameterDial filterSustainDial = { processorRef, "filterSustain", "Filter Sustain" };
    ParameterDial filterReleaseDial = { processorRef, "filterRelease", "Filter Release" };

    void handleNoteOn (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float /*velocity*/) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
