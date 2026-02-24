#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      keyboardComponent (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    // Set up modulation mode state
    modModeState.setModTree (&processorRef.modTree);
    modModeState.addListener (this);

    // Mode label
    modeLabel.setText ("Normal Mode", juce::dontSendNotification);
    modeLabel.setColour (juce::Label::textColourId, TEXT_COLOR);
    modeLabel.setFont (juce::Font (14.0f));
    modeLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (modeLabel);

    processorRef.keyboardState.addListener (this);

    // Create tab content components
    mainTab = std::make_unique<MainTabContent> (p, &modModeState);
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

    setWantsKeyboardFocus (true);
    startTimer (500);

    setSize (windowWidth, windowHeight);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
    modModeState.removeListener (this);
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

    // Position mode label to the right of the tab bar
    auto& tabBar = tabbedComponent.getTabbedButtonBar();
    int tabBarRight = 0;
    for (int i = 0; i < tabBar.getNumTabs(); ++i)
    {
        if (auto* btn = tabBar.getTabButton (i))
            tabBarRight = juce::jmax (tabBarRight, btn->getRight());
    }
    auto tabBarBounds = tabBar.getBounds();
    modeLabel.setBounds (tabBarRight + 15,
        tabBarBounds.getY(),
        200,
        tabBarBounds.getHeight());
}

bool PluginEditor::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress ('z', juce::ModifierKeys::commandModifier, 0))
    {
        processorRef.undoManager.undo();
        return true;
    }
    if (key == juce::KeyPress ('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0))
    {
        processorRef.undoManager.redo();
        return true;
    }
    if (key == juce::KeyPress ('m'))
    {
        modModeState.toggleMode();
        return true;
    }
    if (key == juce::KeyPress (juce::KeyPress::escapeKey))
    {
        if (modModeState.isModulationMode())
        {
            modModeState.setMode (ModulationModeState::Mode::Normal);
            return true;
        }
    }
    return false;
}

void PluginEditor::timerCallback()
{
    if (processorRef.activeGestureCount == 0)
        processorRef.undoManager.beginNewTransaction();
}

void PluginEditor::modulationModeChanged (ModulationModeState::Mode newMode)
{
    if (newMode == ModulationModeState::Mode::Modulation)
    {
        modeLabel.setText ("Modulation Mode", juce::dontSendNotification);
        modeLabel.setColour (juce::Label::textColourId, MOD_COLOR);
    }
    else
    {
        modeLabel.setText ("Normal Mode", juce::dontSendNotification);
        modeLabel.setColour (juce::Label::textColourId, TEXT_COLOR);
    }
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
