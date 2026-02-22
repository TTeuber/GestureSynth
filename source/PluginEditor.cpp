#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      keyboardComponent (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    processorRef.keyboardState.addListener (this);

    // Create tab content components
    mainTab = std::make_unique<MainTabContent> (p);
    oscillatorTab = std::make_unique<OscillatorTabContent> (p);
    modulationTab = std::make_unique<ModulationTabContent> (p);
    effectsTab = std::make_unique<EffectsTabContent> (p);
    experimentTab = std::make_unique<ExperimentTabContent> (p);

    // Add tabs
    tabbedComponent.addTab ("Main", PRIMARY_COLOR, mainTab.get(), false);
    tabbedComponent.addTab ("Oscillator", PRIMARY_COLOR, oscillatorTab.get(), false);
    tabbedComponent.addTab ("Modulation", PRIMARY_COLOR, modulationTab.get(), false);
    tabbedComponent.addTab ("Effects", PRIMARY_COLOR, effectsTab.get(), false);
    tabbedComponent.addTab ("Experiment", PRIMARY_COLOR, experimentTab.get(), false);

    // Style the tab bar
    auto& tabBar = tabbedComponent.getTabbedButtonBar();
    tabBar.setColour (juce::TabbedButtonBar::tabOutlineColourId, juce::Colours::transparentBlack);
    tabBar.setColour (juce::TabbedButtonBar::tabTextColourId, TEXT_COLOR);

    addAndMakeVisible (tabbedComponent);
    addAndMakeVisible (keyboardComponent);

    setSize (windowWidth, windowHeight);
}

PluginEditor::~PluginEditor()
{
    processorRef.keyboardState.removeListener (this);
}

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (PRIMARY_COLOR);
}

void PluginEditor::resized()
{
    juce::Rectangle<int> area = getLocalBounds();
    const juce::Rectangle<int> keyboardContainer = area.removeFromBottom (100);
    keyboardComponent.setBounds (keyboardContainer.reduced (10));

    tabbedComponent.setBounds (area);
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
