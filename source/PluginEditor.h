#pragma once

#include "ADSRGraph.h"
#include "DetuneComponent.h"
#include "FilterDisplay.h"
#include "MatrixComponent.h"
#include "OscGraph.h"
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

    WaveformComponent waveformComponent;
    DetuneComponent detuneComponent;

    ParameterDial volumeDial = { processorRef, "volume", "Volume" };

    ParameterDial subDial = { processorRef, "subOsc", "Sub Oscillator" };
    // ParameterDial pulseWidthDial = { processorRef, "pulseWidth", "Pulse Width" };

    // ParameterDial waveformDial = { processorRef, "oscWaveform", "Waveform" };
    ParameterDial chorusDial = { processorRef, "chorusMix", "Chorus Mix" };

    ADSRGraph ampADSRGraph = { processorRef.parameters, "env1Attack", "env1AttackCurve", "env1Decay", "env1DecayCurve", "env1Sustain", "env1Release", "env1ReleaseCurve", processorRef.getSynth().getAmpADSRPtr() };
    Oscilloscope oscilloscope = Oscilloscope { processorRef };

    FilterDisplay filterDisplay = FilterDisplay { processorRef.parameters };

    void handleNoteOn (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float /*velocity*/) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
