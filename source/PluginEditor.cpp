#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      modWheel (p, &modModeState, &animationSource),
      pitchWheel (p, &animationSource),
      keyboard (p.keyboardState, &animationSource),
      pitchBendRangeControl (p.parameters.getParameter (ParamIDs::pitchBendRange)),
      voiceCountControl (dynamic_cast<juce::AudioParameterChoice*> (p.parameters.getParameter (ParamIDs::voiceCount))),
      monoToggle (dynamic_cast<juce::AudioParameterBool*> (p.parameters.getParameter (ParamIDs::monoOn)), "Mono"),
      legatoToggle (dynamic_cast<juce::AudioParameterBool*> (p.parameters.getParameter (ParamIDs::legatoOn)), "Legato"),
      gateToggle (dynamic_cast<juce::AudioParameterBool*> (p.parameters.getParameter (ParamIDs::gateMode)), "Gate"),
      keyVelComponent (p.parameters, &p.undoManager, &p.activeGestureCount, &modModeState, p.getSynth().getVelocityRawPtr(), p.getSynth().getKeyboardRawPtr(), &animationSource),
      lfoComponent (p.lfoData[0], p.parameters, true, 1),
      adsrGraph (p.parameters, ParamIDs::envParamID (1, "Attack"), ParamIDs::envParamID (1, "AttackCurve"), ParamIDs::envParamID (1, "Decay"), ParamIDs::envParamID (1, "DecayCurve"), ParamIDs::envParamID (1, "Sustain"), ParamIDs::envParamID (1, "Release"), ParamIDs::envParamID (1, "ReleaseCurve"), p.getSynth().getAmpADSRPtr(), &p.undoManager, &p.activeGestureCount, &animationSource)
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
    mainTab = std::make_unique<MainTabContent> (p, &modModeState, &animationSource);
    keyboardTab = std::make_unique<KeyboardTabContent> (p, &animationSource);
    modulationTab = std::make_unique<ModulationTabContent> (p, &animationSource);
    effectsTab = std::make_unique<EffectsTabContent> (p, &modModeState, &animationSource);
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

    // Persistent panel components
    persistentPanel.addAndMakeVisible (modWheel);
    persistentPanel.addAndMakeVisible (pitchWheel);
    persistentPanel.addAndMakeVisible (keyboard);
    persistentPanel.addAndMakeVisible (pitchBendRangeControl);
    persistentPanel.addAndMakeVisible (voiceCountControl);
    persistentPanel.addAndMakeVisible (monoToggle);
    persistentPanel.addAndMakeVisible (legatoToggle);
    persistentPanel.addAndMakeVisible (gateToggle);
    persistentPanel.addAndMakeVisible (keyVelComponent);
    persistentPanel.addAndMakeVisible (lfoComponent);
    persistentPanel.addAndMakeVisible (adsrGraph);

    // MW / AT / EXP tabs
    mwTab.setup ("MW", "modWheel", &modModeState, [this]
    {
        if (&modModeState != nullptr)
        {
            modModeState.setTargetSource ("modWheel");
            if (!modModeState.isModulationMode())
                modModeState.setMode (ModulationModeState::Mode::Modulation);
        }
    });
    atTab.setup ("AT", "aftertouch", &modModeState, [this]
    {
        if (&modModeState != nullptr)
        {
            modModeState.setTargetSource ("aftertouch");
            if (!modModeState.isModulationMode())
                modModeState.setMode (ModulationModeState::Mode::Modulation);
        }
    });
    expTab.setup ("EXP", "expression", &modModeState, [this]
    {
        if (&modModeState != nullptr)
        {
            modModeState.setTargetSource ("expression");
            if (!modModeState.isModulationMode())
                modModeState.setMode (ModulationModeState::Mode::Modulation);
        }
    });
    mwTab.setCompactMode (true);
    atTab.setCompactMode (true);
    expTab.setCompactMode (true);
    persistentPanel.addAndMakeVisible (mwTab);
    persistentPanel.addAndMakeVisible (atTab);
    persistentPanel.addAndMakeVisible (expTab);

    const juce::StringArray lfoIDs = { "lfo1", "lfo2", "lfo3", "lfo4" };
    const juce::StringArray envIDs = { "env1", "env2", "env3", "env4" };

    for (int i = 0; i < 4; ++i)
    {
        lfoTabs[i].setup ("LFO " + juce::String (i + 1), lfoIDs[i], &modModeState,
            [this, i]() { selectLfo (i); });
        persistentPanel.addAndMakeVisible (lfoTabs[i]);

        envTabs[i].setup ("ENV " + juce::String (i + 1), envIDs[i], &modModeState,
            [this, i]() { selectEnv (i); });
        persistentPanel.addAndMakeVisible (envTabs[i]);
    }

    lfoTabs[0].setSelected (true);
    envTabs[0].setSelected (true);

    velTab.setup ("Vel", "velocity", &modModeState, [this] { selectKeyVel (0); });
    keyTab.setup ("Key", "keyboard", &modModeState, [this] { selectKeyVel (1); });
    persistentPanel.addAndMakeVisible (velTab);
    persistentPanel.addAndMakeVisible (keyTab);
    velTab.setSelected (true);

    contentWrapper.addAndMakeVisible (persistentPanel);
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
    animationSource.start();

    setSize (WIDTH, HEIGHT);
}

PluginEditor::~PluginEditor()
{
    animationSource.stop();
    stopTimer();
    modModeState.removeListener (this);
    processorRef.keyboardState.removeListener (this);
}

void PluginEditor::selectLfo (int index)
{
    activeLfoIndex = index;
    for (int i = 0; i < 4; ++i)
        lfoTabs[i].setSelected (i == index);
    lfoComponent.rebind (processorRef.lfoData[index], index + 1);
}

void PluginEditor::selectEnv (int index)
{
    activeEnvIndex = index;
    for (int i = 0; i < 4; ++i)
        envTabs[i].setSelected (i == index);
    int envNum = index + 1;
    auto adsrPtr = (index == 0) ? processorRef.getSynth().getAmpADSRPtr() : std::make_shared<MyADSR*> (nullptr);
    adsrGraph.rebind (
        ParamIDs::envParamID (envNum, "Attack"),
        ParamIDs::envParamID (envNum, "AttackCurve"),
        ParamIDs::envParamID (envNum, "Decay"),
        ParamIDs::envParamID (envNum, "DecayCurve"),
        ParamIDs::envParamID (envNum, "Sustain"),
        ParamIDs::envParamID (envNum, "Release"),
        ParamIDs::envParamID (envNum, "ReleaseCurve"),
        adsrPtr);
}

void PluginEditor::selectKeyVel (int index)
{
    activeKeyVelTab = index;
    velTab.setSelected (index == 0);
    keyTab.setSelected (index == 1);
    keyVelComponent.setActiveTab (index);
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

    constexpr int persistentHeight = 380;
    tabbedComponent.setBounds (0, 0, WIDTH, HEIGHT - persistentHeight);
    persistentPanel.setBounds (0, HEIGHT - persistentHeight, WIDTH, persistentHeight);

    // Layout persistent panel
    auto panelArea = persistentPanel.getLocalBounds();

    // Bottom: keyboard row (~160px)
    constexpr int keyboardRowHeight = 160;
    auto keyboardRow = panelArea.removeFromBottom (keyboardRowHeight);

    int wheelWidth = juce::jmax (36, keyboardRow.getHeight() / 3);
    auto wheelsArea = keyboardRow.removeFromLeft (wheelWidth * 2 + 4);
    modWheel.setBounds (wheelsArea.removeFromLeft (wheelWidth).reduced (2));
    pitchWheel.setBounds (wheelsArea.removeFromLeft (wheelWidth).reduced (2));

    int kvWidth = keyboardRow.getHeight() + 10;
    auto kvColumn = keyboardRow.removeFromRight (kvWidth);
    keyVelComponent.setBounds (kvColumn.reduced (5));

    constexpr int controlRowHeight = 28;
    auto controlRow = keyboardRow.removeFromBottom (controlRowHeight);

    int controlWidth = controlRow.getWidth();
    int pbWidth = controlWidth * 25 / 100;
    int vcWidth = controlWidth * 20 / 100;
    int toggleWidth = (controlWidth - pbWidth - vcWidth) / 3;

    pitchBendRangeControl.setBounds (controlRow.removeFromLeft (pbWidth).reduced (2, 2));
    voiceCountControl.setBounds (controlRow.removeFromLeft (vcWidth).reduced (2, 2));
    monoToggle.setBounds (controlRow.removeFromLeft (toggleWidth).reduced (4, 3));
    legatoToggle.setBounds (controlRow.removeFromLeft (toggleWidth).reduced (4, 3));
    gateToggle.setBounds (controlRow.reduced (4, 3));

    keyboard.setBounds (keyboardRow.reduced (2));

    // Top: LFO/ADSR row with tab strip
    auto row3 = panelArea;
    auto buttonRow = row3.removeFromBottom (30).reduced (5, 0);

    int contentHalf = row3.getWidth() / 2;
    lfoComponent.setBounds (row3.removeFromLeft (contentHalf).reduced (5));
    adsrGraph.setBounds (row3.reduced (5));

    // Tab strip
    int tabWidth = buttonRow.getWidth() / 11;
    int narrowTab = 3 * tabWidth / 5;

    mwTab.setBounds (buttonRow.removeFromLeft (narrowTab).reduced (1, 0));
    atTab.setBounds (buttonRow.removeFromLeft (narrowTab).reduced (1, 0));
    expTab.setBounds (buttonRow.removeFromLeft (narrowTab).reduced (1, 0));
    for (int i = 0; i < 4; ++i)
        lfoTabs[i].setBounds (buttonRow.removeFromLeft (tabWidth).reduced (1, 0));
    for (int i = 0; i < 4; ++i)
        envTabs[i].setBounds (buttonRow.removeFromLeft (tabWidth).reduced (1, 0));
    velTab.setBounds (buttonRow.removeFromLeft (narrowTab).reduced (1, 0));
    keyTab.setBounds (buttonRow.removeFromLeft (narrowTab).reduced (1, 0));

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

    // Repaint mod source tabs
    for (int i = 0; i < 4; ++i)
    {
        lfoTabs[i].repaint();
        envTabs[i].repaint();
    }
    mwTab.repaint();
    atTab.repaint();
    expTab.repaint();
    velTab.repaint();
    keyTab.repaint();
    modWheel.repaint();
}

void PluginEditor::targetSourceChanged (const juce::String&)
{
    for (int i = 0; i < 4; ++i)
    {
        lfoTabs[i].repaint();
        envTabs[i].repaint();
    }
    mwTab.repaint();
    atTab.repaint();
    expTab.repaint();
    velTab.repaint();
    keyTab.repaint();
    modWheel.repaint();
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
