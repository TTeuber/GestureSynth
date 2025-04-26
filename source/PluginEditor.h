#pragma once

#include "ADSRGraph.h"
#include "MatrixComponent.h"
#include "Oscilliscope.h"
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
    ParameterDial filterCutoffDial = { processorRef, "filterFrequency", "Filter Cutoff" };
    ParameterDial filterResonanceDial = { processorRef, "filterResonance", "Filter Resonance" };
    ParameterDial pulseWidthDial = { processorRef, "pulseWidth", "Pulse Width" };
    ParameterDial env1AttackDial = { processorRef, "env1Attack", "Envelope 1 Attack" };
    ParameterDial env1AttackCurveDial = { processorRef, "env1AttackCurve", "Envelope 1 Attack Curve" };
    ParameterDial env1DecayDial = { processorRef, "env1Decay", "Envelope 1 Decay" };
    ParameterDial env1DecayCurveDial = { processorRef, "env1DecayCurve", "Envelope 1 Decay Curve" };
    ParameterDial env1SustainDial = { processorRef, "env1Sustain", "Envelope 1 Sustain" };
    ParameterDial env1ReleaseDial = { processorRef, "env1Release", "Envelope 1 Release" };
    ParameterDial env1ReleaseCurveDial = { processorRef, "env1ReleaseCurve", "Envelope 1 Release Curve" };

    ADSRGraph ampADSRGraph = { processorRef.parameters, "env1Attack", "env1AttackCurve", "env1Decay", "env1DecayCurve", "env1Sustain", "env1Release", "env1ReleaseCurve", processorRef.getSynth().getAmpADSRPtr() };
    Oscilloscope oscilloscope = Oscilloscope { processorRef };

    void handleNoteOn (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float /*velocity*/) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
