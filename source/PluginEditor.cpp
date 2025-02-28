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

    setSize (800, 600);
}

PluginEditor::~PluginEditor()
{
    processorRef.keyboardState.removeListener (this);
}

void PluginEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (16.0f);
}

void PluginEditor::resized()
{
    juce::Rectangle<int> area = getLocalBounds();
    area.removeFromBottom (50);
    keyboardComponent.setBounds (10, getHeight() - 80, getWidth() - 20, 70);

    constexpr int containerHeight = 120;

    juce::Rectangle<int> containerA = area.removeFromTop (containerHeight);
    const int sectionAWidth = containerA.getWidth() / 4;

    juce::Rectangle<int> containerB = area.removeFromTop (containerHeight);
    const int sectionBWidth = containerB.getWidth() / 4;

    juce::Rectangle<int> containerC = area.removeFromTop (containerHeight);
    const int sectionCWidth = containerC.getWidth() / 4;

    volumeDial.setBounds (containerA.removeFromLeft (sectionAWidth));
    filterFrequencyDial.setBounds (containerA.removeFromLeft (sectionAWidth));
    filterEnvelopeAmountDial.setBounds (containerA.removeFromLeft (sectionAWidth));
    filterResonanceDial.setBounds (containerA.removeFromLeft (sectionAWidth));

    ampAttackDial.setBounds (containerB.removeFromLeft (sectionBWidth));
    ampDecayDial.setBounds (containerB.removeFromLeft (sectionBWidth));
    ampSustainDial.setBounds (containerB.removeFromLeft (sectionBWidth));
    ampReleaseDial.setBounds (containerB.removeFromLeft (sectionBWidth));

    filterAttackDial.setBounds (containerC.removeFromLeft (sectionCWidth));
    filterDecayDial.setBounds (containerC.removeFromLeft (sectionCWidth));
    filterSustainDial.setBounds (containerC.removeFromLeft (sectionCWidth));
    filterReleaseDial.setBounds (containerC.removeFromLeft (sectionCWidth));
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
