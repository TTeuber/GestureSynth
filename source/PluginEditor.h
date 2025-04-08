#pragma once

#include "ADSRGraph.h"
#include "MatrixComponent.h"
#include "ParameterDial.h"
#include "PluginProcessor.h"
#include "Theme.h"
#include <juce_audio_utils/juce_audio_utils.h>

#define HEIGHT 800
#define WIDTH 1000

//==============================================================================
class PluginEditor final : public juce::AudioProcessorEditor, public juce::MidiKeyboardStateListener
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    int windowHeight = HEIGHT;
    int windowWidth = WIDTH;

    PluginProcessor& processorRef;

    juce::MidiKeyboardComponent keyboardComponent;

    MatrixComponent matrixComponent;

    ParameterDial volumeDial = { processorRef, "volume", "Volume" };
    ParameterDial ampAttackDial = { processorRef, "ampAttack", "Amp Attack" };
    ParameterDial ampAttackCurveDial = { processorRef, "ampAttackCurve", "Amp Attack Curve" };
    ParameterDial ampDecayDial = { processorRef, "ampDecay", "Amp Decay" };
    ParameterDial ampDecayCurveDial = { processorRef, "ampDecayCurve", "Amp Decay Curve" };
    ParameterDial ampSustainDial = { processorRef, "ampSustain", "Amp Sustain" };
    ParameterDial ampReleaseDial = { processorRef, "ampRelease", "Amp Release" };
    ParameterDial ampReleaseCurveDial = { processorRef, "ampReleaseCurve", "Amp Release Curve" };
    ParameterDial filterFrequencyDial = { processorRef, "filterFrequency", "Filter Frequency" };
    ParameterDial filterEnvelopeAmountDial = { processorRef, "filterEnvelopeAmount", "Filter Envelope Amount" };
    ParameterDial filterResonanceDial = { processorRef, "filterResonance", "Filter Resonance" };
    ParameterDial filterAttackDial = { processorRef, "filterAttack", "Filter Attack" };
    ParameterDial filterDecayDial = { processorRef, "filterDecay", "Filter Decay" };
    ParameterDial filterSustainDial = { processorRef, "filterSustain", "Filter Sustain" };
    ParameterDial filterReleaseDial = { processorRef, "filterRelease", "Filter Release" };

    ADSRGraph ampADSRGraph = { processorRef.parameters, "ampAttack", "ampAttackCurve", "ampDecay", "ampDecayCurve", "ampSustain", "ampRelease", "ampReleaseCurve", processorRef.getSynth().getAmpADSRPtr() };
    ADSRGraph filterADSRGraph = { processorRef.parameters, "filterAttack", "filterAttackCurve", "filterDecay", "filterDecayCurve", "filterSustain", "filterRelease", "filterReleaseCurve", processorRef.getSynth().getFilterADSRPtr() };

    void handleNoteOn (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float /*velocity*/) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
