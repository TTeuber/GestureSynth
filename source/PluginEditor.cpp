#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      keyboardComponent (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard),
      matrixComponent (p.modTree)
{
    processorRef.keyboardState.addListener (this);

    addAndMakeVisible (keyboardComponent);

    addAndMakeVisible (matrixComponent);

    addAndMakeVisible (volumeDial);

    addAndMakeVisible (env1AttackDial);
    addAndMakeVisible (env1AttackCurveDial);
    addAndMakeVisible (env1DecayDial);
    addAndMakeVisible (env1DecayCurveDial);
    addAndMakeVisible (env1SustainDial);
    addAndMakeVisible (env1ReleaseDial);
    addAndMakeVisible (env1ReleaseCurveDial);

    addAndMakeVisible (ampADSRGraph);

    setSize (windowWidth, windowHeight);
}

PluginEditor::~PluginEditor()
{
    processorRef.keyboardState.removeListener (this);
}

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (PRIMARY_COLOR);

    g.setColour (TEXT_COLOR);
    g.setFont (16.0f);
}

void PluginEditor::resized()
{
    juce::Rectangle<int> area = getLocalBounds();
    const juce::Rectangle<int> keyboardContainer = area.removeFromBottom (100);
    keyboardComponent.setBounds (keyboardContainer.reduced (10));

    const juce::Rectangle<int> matrixContainer = area.removeFromRight (300);
    matrixComponent.setBounds (matrixContainer.reduced (10));

    const int containerHeight = area.getHeight() / 8;

    juce::Rectangle<int> basicContainer = area.removeFromTop (containerHeight);
    const int sectionAWidth = basicContainer.getWidth() / 4;

    const juce::Rectangle<int> ampADSRContainer = area.removeFromTop (static_cast<int> (containerHeight * 1.5));
    ampADSRGraph.setBounds (ampADSRContainer.reduced (10));

    juce::Rectangle<int> ampDialContainer = area.removeFromTop (containerHeight);
    const int sectionBWidth = ampDialContainer.getWidth() / 4;

    juce::Rectangle<int> ampCurveDialContainer = area.removeFromTop (containerHeight);
    const int sectionDWidth = ampCurveDialContainer.getWidth() / 3;

    volumeDial.setBounds (basicContainer.removeFromLeft (sectionAWidth));

    env1AttackDial.setBounds (ampDialContainer.removeFromLeft (sectionBWidth));
    env1DecayDial.setBounds (ampDialContainer.removeFromLeft (sectionBWidth));
    env1SustainDial.setBounds (ampDialContainer.removeFromLeft (sectionBWidth));
    env1ReleaseDial.setBounds (ampDialContainer.removeFromLeft (sectionBWidth));

    env1AttackCurveDial.setBounds (ampCurveDialContainer.removeFromLeft (sectionDWidth));
    env1DecayCurveDial.setBounds (ampCurveDialContainer.removeFromLeft (sectionDWidth));
    env1ReleaseCurveDial.setBounds (ampCurveDialContainer.removeFromLeft (sectionDWidth));
}

void PluginEditor::handleNoteOn (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
    const juce::MidiMessage message = juce::MidiMessage::noteOn (midiChannel, midiNoteNumber, velocity);
    processorRef.getMidiMessages().addEvent (message, 0);
}

void PluginEditor::handleNoteOff (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float)
{
    const juce::MidiMessage message = juce::MidiMessage::noteOff (midiChannel, midiNoteNumber);
    processorRef.getMidiMessages().addEvent (message, 0);
}
