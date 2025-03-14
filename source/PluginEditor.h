#pragma once

#include "ADSRGraph.h"
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
    int windowHeight = 800;
    int windowWidth = 800;

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

    ADSRGraph ampADSRGraph = { processorRef.parameters, "ampAttack", "ampDecay", "ampSustain", "ampRelease", processorRef.getSynth().getAmpADSRPtr() };
    ADSRGraph filterADSRGraph = { processorRef.parameters, "filterAttack", "filterDecay", "filterSustain", "filterRelease", processorRef.getSynth().getFilterADSRPtr() };

    void handleNoteOn (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float /*velocity*/) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
