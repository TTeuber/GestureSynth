#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p)
{
    // Set up modulation mode state
    modModeState.setModTree (&processorRef.modTree);
    modModeState.setUndoManager (&processorRef.undoManager);
    modModeState.addListener (this);

    // Mode label — child of contentWrapper
    modeLabel.setText ("Normal Mode", juce::dontSendNotification);
    modeLabel.setColour (juce::Label::textColourId, TEXT_COLOR);
    modeLabel.setFont (juce::Font (14.0f));
    modeLabel.setJustificationType (juce::Justification::centredLeft);
    contentWrapper.addAndMakeVisible (modeLabel);

    // Scale label — child of contentWrapper
    scaleLabel.setText ("100%", juce::dontSendNotification);
    scaleLabel.setColour (juce::Label::textColourId, TEXT_COLOR.withAlpha (0.5f));
    scaleLabel.setFont (juce::Font (12.0f));
    scaleLabel.setJustificationType (juce::Justification::centredRight);
    contentWrapper.addAndMakeVisible (scaleLabel);

    processorRef.keyboardState.addListener (this);

    // Create tab content components
    mainTab = std::make_unique<MainTabContent> (p, &modModeState);
    keyboardTab = std::make_unique<KeyboardTabContent> (p);
    modulationTab = std::make_unique<ModulationTabContent> (p);
    effectsTab = std::make_unique<EffectsTabContent> (p);
    experimentTab = std::make_unique<ExperimentTabContent> (p);

    // Add tabs
    tabbedComponent.addTab ("Main", PRIMARY_COLOR, mainTab.get(), false);
    tabbedComponent.addTab ("Keyboard", PRIMARY_COLOR, keyboardTab.get(), false);
    tabbedComponent.addTab ("Modulation", PRIMARY_COLOR, modulationTab.get(), false);
    tabbedComponent.addTab ("Effects", PRIMARY_COLOR, effectsTab.get(), false);
    tabbedComponent.addTab ("Experiment", PRIMARY_COLOR, experimentTab.get(), false);

    // Style the tab bar
    auto& tabBar = tabbedComponent.getTabbedButtonBar();
    tabBar.setColour (juce::TabbedButtonBar::tabOutlineColourId, juce::Colours::transparentBlack);
    tabBar.setColour (juce::TabbedButtonBar::tabTextColourId, TEXT_COLOR);

    contentWrapper.addAndMakeVisible (tabbedComponent);
    addAndMakeVisible (contentWrapper);

    // Constrainer: fixed aspect ratio, min/max scale
    constrainer.setFixedAspectRatio ((double) WIDTH / (double) HEIGHT);
    constrainer.setSizeLimits ((int) (WIDTH * kMinScale), (int) (HEIGHT * kMinScale),
                               (int) (WIDTH * kMaxScale), (int) (HEIGHT * kMaxScale));

    setResizable (true, false);
    setConstrainer (&constrainer);

    // Resize grip — direct child of editor
    resizeGrip = std::make_unique<ResizeGrip> (this, &constrainer);
    addAndMakeVisible (*resizeGrip);

    setWantsKeyboardFocus (true);
    startTimer (500);

    setSize (WIDTH, HEIGHT);
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
    scaleFactor = (float) getWidth() / (float) WIDTH;

    contentWrapper.setTransform (juce::AffineTransform::scale (scaleFactor));
    contentWrapper.setBounds (0, 0, WIDTH, HEIGHT);

    tabbedComponent.setBounds (0, 0, WIDTH, HEIGHT);

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

    // Scale label in top-right of tab bar area
    scaleLabel.setBounds (WIDTH - 80, tabBarBounds.getY(), 70, tabBarBounds.getHeight());
    int scalePercent = juce::roundToInt (scaleFactor * 100.0f);
    scaleLabel.setText (juce::String (scalePercent) + "%", juce::dontSendNotification);

    // Resize grip at physical bottom-right corner (not inside contentWrapper)
    resizeGrip->setBounds (getWidth() - 16, getHeight() - 16, 16, 16);
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
