#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      keyboardComponent (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard),
      matrixComponent (p.modTree),
      waveformComponent (p.parameters),
      detuneComponent (p.parameters),
      subOscillatorComponent (p.parameters),
      chorusComponent (p.parameters)
{
    processorRef.keyboardState.addListener (this);

    addAndMakeVisible (keyboardComponent);

    addAndMakeVisible (matrixComponent);

    addAndMakeVisible (volumeComponent);
    addAndMakeVisible (chorusMixComponent);

    addAndMakeVisible (ampADSRGraph);
    addAndMakeVisible (oscilloscope);
    addAndMakeVisible (filterDisplay);
    addAndMakeVisible (lfoComponent);

    addAndMakeVisible (waveformComponent);
    addAndMakeVisible (detuneComponent);
    addAndMakeVisible (subOscillatorComponent);
    addAndMakeVisible (chorusComponent);

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

    const int containerHeight = area.getHeight() / 8;

    juce::Rectangle<int> matrixContainer = area.removeFromRight (300);
    juce::Rectangle<int> dialContainerH = matrixContainer.removeFromBottom (containerHeight);
    const int dialWidth = matrixContainer.getWidth() / 4;
    volumeComponent.setBounds (dialContainerH.removeFromLeft (dialWidth).reduced (10));
    chorusMixComponent.setBounds (dialContainerH.removeFromLeft (dialWidth).reduced (10));
    matrixComponent.setBounds (matrixContainer.reduced (10));

    juce::Rectangle<int> dialContainerV = area.removeFromLeft (containerHeight * 2);
    waveformComponent.setBounds (dialContainerV.removeFromTop (containerHeight * 2).reduced (10));
    detuneComponent.setBounds (dialContainerV.removeFromTop (containerHeight * 2).reduced (10));
    subOscillatorComponent.setBounds (dialContainerV.removeFromTop (containerHeight * 2).reduced (10));
    chorusComponent.setBounds (dialContainerV.removeFromTop (containerHeight * 2).reduced (10));

    const juce::Rectangle<int> ampADSRContainer = area.removeFromTop (containerHeight * 2);
    ampADSRGraph.setBounds (ampADSRContainer.reduced (10));

    const juce::Rectangle<int> filterDisplayContainer = area.removeFromTop (containerHeight * 2);
    filterDisplay.setBounds (filterDisplayContainer.reduced (10));

    const juce::Rectangle<int> oscilloscopeContainer = area.removeFromTop (containerHeight * 2);
    oscilloscope.setBounds (oscilloscopeContainer.reduced (10));

    const juce::Rectangle<int> filterContainer = area.removeFromTop (containerHeight * 2);
    lfoComponent.setBounds (filterContainer.reduced (10));
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
