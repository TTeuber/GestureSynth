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
    addAndMakeVisible (filterFrequencyDial);
    addAndMakeVisible (filterEnvelopeAmountDial);
    addAndMakeVisible (filterResonanceDial);

    addAndMakeVisible (ampAttackDial);
    addAndMakeVisible (ampAttackCurveDial);
    addAndMakeVisible (ampDecayDial);
    addAndMakeVisible (ampDecayCurveDial);
    addAndMakeVisible (ampSustainDial);
    addAndMakeVisible (ampReleaseDial);
    addAndMakeVisible (ampReleaseCurveDial);

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
    g.fillAll (PRIMARY_COLOR);

    g.setColour (TEXT_COLOR);
    g.setFont (16.0f);
}

void PluginEditor::resized()
{
    juce::Rectangle<int> area = getLocalBounds();
    juce::Rectangle<int> keyboardContainer = area.removeFromBottom (100);
    keyboardComponent.setBounds (keyboardContainer.reduced (10));

    juce::Rectangle<int> matrixContainer = area.removeFromRight (300);
    matrixComponent.setBounds (matrixContainer.reduced (10));

    const int containerHeight = area.getHeight() / 8;

    juce::Rectangle<int> basicContainer = area.removeFromTop (containerHeight);
    const int sectionAWidth = basicContainer.getWidth() / 4;

    const juce::Rectangle<int> ampADSRContainer = area.removeFromTop (containerHeight * 1.5);
    ampADSRGraph.setBounds (ampADSRContainer.reduced (10));

    juce::Rectangle<int> ampDialContainer = area.removeFromTop (containerHeight);
    const int sectionBWidth = ampDialContainer.getWidth() / 4;

    juce::Rectangle<int> ampCurveDialContainer = area.removeFromTop (containerHeight);
    const int sectionDWidth = ampCurveDialContainer.getWidth() / 3;

    juce::Rectangle<int> filterADSRContainer = area.removeFromTop (containerHeight * 1.5);
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

    ampAttackCurveDial.setBounds (ampCurveDialContainer.removeFromLeft (sectionDWidth));
    ampDecayCurveDial.setBounds (ampCurveDialContainer.removeFromLeft (sectionDWidth));
    ampReleaseCurveDial.setBounds (ampCurveDialContainer.removeFromLeft (sectionDWidth));

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
