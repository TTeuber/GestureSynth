#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), keyboardComponent (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    processorRef.keyboardState.addListener (this);

    addAndMakeVisible (keyboardComponent);

    addAndMakeVisible (volumeDial);
    addAndMakeVisible (filterFrequencyDial);
    addAndMakeVisible (filterEnvelopeAmountDial);
    addAndMakeVisible (filterResonanceDial);

    addAndMakeVisible (ampAttackDial);
    addAndMakeVisible (ampDecayDial);
    addAndMakeVisible (ampSustainDial);
    addAndMakeVisible (ampReleaseDial);

    addAndMakeVisible (filterAttackDial);
    addAndMakeVisible (filterDecayDial);
    addAndMakeVisible (filterSustainDial);
    addAndMakeVisible (filterReleaseDial);

    addAndMakeVisible (ampADSRGraph);
    addAndMakeVisible (filterADSRGraph);

    setSize (windowWidth, windowHeight);
}

PluginEditor::~PluginEditor()
{
    processorRef.keyboardState.removeListener (this);
}

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (16.0f);
}

void PluginEditor::resized()
{
    juce::Rectangle<int> area = getLocalBounds();
    area.removeFromBottom (50);
    keyboardComponent.setBounds (10, getHeight() - 80, getWidth() - 20, 70);

    const int containerHeight = area.getHeight() / 8;
    juce::Rectangle<int> newArea = area.removeFromRight (windowWidth / 2);

    juce::Rectangle<int> basicContainer = area.removeFromTop (containerHeight);
    const int sectionAWidth = basicContainer.getWidth() / 4;

    juce::Rectangle<int> ampADSRContainer = area.removeFromTop (containerHeight);
    ampADSRGraph.setBounds (ampADSRContainer.reduced (10));

    juce::Rectangle<int> ampDialContainer = area.removeFromTop (containerHeight);
    const int sectionBWidth = ampDialContainer.getWidth() / 4;

    juce::Rectangle<int> filterADSRContainer = area.removeFromTop (containerHeight);
    filterADSRGraph.setBounds (filterADSRContainer.reduced (10));

    juce::Rectangle<int> filterDialContainer = area.removeFromTop (containerHeight);
    const int sectionCWidth = filterDialContainer.getWidth() / 4;

    volumeDial.setBounds (basicContainer.removeFromLeft (sectionAWidth));
    filterFrequencyDial.setBounds (basicContainer.removeFromLeft (sectionAWidth));
    filterEnvelopeAmountDial.setBounds (basicContainer.removeFromLeft (sectionAWidth));
    filterResonanceDial.setBounds (basicContainer.removeFromLeft (sectionAWidth));

    ampAttackDial.setBounds (ampDialContainer.removeFromLeft (sectionBWidth));
    ampDecayDial.setBounds (ampDialContainer.removeFromLeft (sectionBWidth));
    ampSustainDial.setBounds (ampDialContainer.removeFromLeft (sectionBWidth));
    ampReleaseDial.setBounds (ampDialContainer.removeFromLeft (sectionBWidth));

    filterAttackDial.setBounds (filterDialContainer.removeFromLeft (sectionCWidth));
    filterDecayDial.setBounds (filterDialContainer.removeFromLeft (sectionCWidth));
    filterSustainDial.setBounds (filterDialContainer.removeFromLeft (sectionCWidth));
    filterReleaseDial.setBounds (filterDialContainer.removeFromLeft (sectionCWidth));
}

void PluginEditor::handleNoteOn (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
    juce::MidiMessage message = juce::MidiMessage::noteOn (midiChannel, midiNoteNumber, velocity);
    processorRef.getMidiMessages().addEvent (message, 0);
}

void PluginEditor::handleNoteOff (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float)
{
    juce::MidiMessage message = juce::MidiMessage::noteOff (midiChannel, midiNoteNumber);
    processorRef.getMidiMessages().addEvent (message, 0);
}
