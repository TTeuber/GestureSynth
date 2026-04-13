#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      modWheel (p, &modModeState, &animationSource),
      pitchWheel (p, &animationSource),
      keyboard (p.keyboardState, &animationSource),
      keyVelComponent (p.parameters, &p.undoManager, &p.activeGestureCount, &modModeState, p.getSynth().getVelocityRawPtr(), p.getSynth().getKeyboardRawPtr(), &animationSource, p.getSynth().getAmpADSRPtr()),
      pitchBendRangeControl (p.parameters.getParameter (ParamIDs::pitchBendRange)),
      voiceCountControl (dynamic_cast<juce::AudioParameterChoice*> (p.parameters.getParameter (ParamIDs::voiceCount)),
                         dynamic_cast<juce::AudioParameterBool*> (p.parameters.getParameter (ParamIDs::monoOn)),
                         dynamic_cast<juce::AudioParameterBool*> (p.parameters.getParameter (ParamIDs::legatoOn))),
      volumeControl (p.parameters.getParameter (ParamIDs::volume)),
      portamentoBottomControl (p.parameters.getParameter (ParamIDs::portamentoTime)),
      lfoComponent (p.lfoData[0], p.parameters, true, 1, p.getSynth().getLFOPtr (0), &animationSource),
      adsrGraph (p.parameters, ParamIDs::envParamID (1, "Attack"), ParamIDs::envParamID (1, "AttackCurve"), ParamIDs::envParamID (1, "Decay"), ParamIDs::envParamID (1, "DecayCurve"), ParamIDs::envParamID (1, "Sustain"), ParamIDs::envParamID (1, "Release"), ParamIDs::envParamID (1, "ReleaseCurve"), p.getSynth().getAmpADSRPtr(), &p.undoManager, &p.activeGestureCount, &animationSource)
{
    // Set up modulation mode state
    modModeState.setModTree (&processorRef.modTree);
    modModeState.setUndoManager (&processorRef.undoManager);
    modModeState.addListener (this);


    processorRef.keyboardState.addListener (this);

    // Create tab content components
    mainTab = std::make_unique<MainTabContent> (p, &modModeState, &animationSource);
    modulationTab = std::make_unique<ModulationTabContent> (p, &animationSource);

    // Add tabs
    tabbedComponent.addTab ("Main", PRIMARY_COLOR, mainTab.get(), false);
#if SYNTHDEMO_DEV_MODE
    keyboardTab = std::make_unique<KeyboardTabContent> (p, &animationSource);
    tabbedComponent.addTab ("Keyboard", PRIMARY_COLOR, keyboardTab.get(), false);
#endif
    tabbedComponent.addTab ("Modulation", PRIMARY_COLOR, modulationTab.get(), false);
#if SYNTHDEMO_DEV_MODE
    experimentTab = std::make_unique<ExperimentTabContent> (p);
    tabbedComponent.addTab ("Experiment", PRIMARY_COLOR, experimentTab.get(), false);
#endif

    // Style the tab bar
    tabbedComponent.setLookAndFeel (&customLookAndFeel);
    tabbedComponent.setTabBarDepth (30);
    tabbedComponent.setOutline (0);

    contentWrapper.addAndMakeVisible (tabbedComponent);

    // Persistent panel components
    persistentPanel.addAndMakeVisible (wheelBoxBackground);
    persistentPanel.addAndMakeVisible (modWheel);
    persistentPanel.addAndMakeVisible (pitchWheel);
    persistentPanel.addAndMakeVisible (keyboard);
    persistentPanel.addAndMakeVisible (pitchBendRangeControl);
    persistentPanel.addAndMakeVisible (voiceCountControl);
    persistentPanel.addAndMakeVisible (volumeControl);
    persistentPanel.addAndMakeVisible (portamentoBottomControl);

    prevPresetButton.setColour (juce::TextButton::buttonColourId, SECONDARY_COLOR);
    prevPresetButton.setColour (juce::TextButton::textColourOffId, TEXT_COLOR);
    prevPresetButton.onClick = [this] { navigatePreset (-1); };
    contentWrapper.addAndMakeVisible (prevPresetButton);
    prevPresetButton.setAlwaysOnTop (true);

    presetButton.setColour (juce::TextButton::buttonColourId, SECONDARY_COLOR);
    presetButton.setColour (juce::TextButton::textColourOffId, TEXT_COLOR);
    presetButton.onClick = [this]
    {
        std::map<int, juce::File> idToFile;
        auto menu = processorRef.presetManager.buildMenu (idToFile);

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetButton),
            [this, idToFile = std::move (idToFile)] (int result)
            {
                if (result > 0)
                {
                    auto it = idToFile.find (result);
                    if (it != idToFile.end())
                        loadPresetByFile (it->second);
                }
            });
    };
    contentWrapper.addAndMakeVisible (presetButton);
    presetButton.setAlwaysOnTop (true);

    nextPresetButton.setColour (juce::TextButton::buttonColourId, SECONDARY_COLOR);
    nextPresetButton.setColour (juce::TextButton::textColourOffId, TEXT_COLOR);
    nextPresetButton.onClick = [this] { navigatePreset (1); };
    contentWrapper.addAndMakeVisible (nextPresetButton);
    nextPresetButton.setAlwaysOnTop (true);

    panicButton.setColour (juce::TextButton::buttonColourId, SECONDARY_COLOR);
    panicButton.setColour (juce::TextButton::textColourOffId, TEXT_COLOR);
    panicButton.setTooltip ("Silence all voices and reset MIDI state");
    panicButton.onClick = [this] { processorRef.getSynth().allNotesOff (0, false); };
    persistentPanel.addAndMakeVisible (panicButton);

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
    tabbedComponent.setLookAndFeel (nullptr);
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
    lfoComponent.rebind (processorRef.lfoData[index], index + 1, processorRef.getSynth().getLFOPtr (index));
}

void PluginEditor::selectEnv (int index)
{
    activeEnvIndex = index;
    for (int i = 0; i < 4; ++i)
        envTabs[i].setSelected (i == index);
    int envNum = index + 1;
    auto adsrPtr = processorRef.getSynth().getEnvPtr (index);
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

void PluginEditor::loadPresetByFile (const juce::File& file)
{
    auto state = processorRef.presetManager.loadPreset (file);
    if (state.isValid())
    {
        processorRef.restoreFromStateTree (state);
        processorRef.currentPresetName = file.getFileNameWithoutExtension();
        processorRef.currentPresetFile = file;
        presetButton.setButtonText (processorRef.currentPresetName);
    }
}

void PluginEditor::navigatePreset (int direction)
{
    auto flatList = processorRef.presetManager.getFlatPresetList();
    if (flatList.empty())
        return;

    int currentIndex = -1;
    for (int i = 0; i < (int) flatList.size(); ++i)
    {
        if (flatList[i].file == processorRef.currentPresetFile)
        {
            currentIndex = i;
            break;
        }
    }

    int newIndex;
    if (currentIndex < 0)
        newIndex = (direction > 0) ? 0 : (int) flatList.size() - 1;
    else
        newIndex = (currentIndex + direction + (int) flatList.size()) % (int) flatList.size();

    loadPresetByFile (flatList[newIndex].file);
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

    constexpr int persistentHeight = 408;
    tabbedComponent.setBounds (0, 0, WIDTH, HEIGHT - persistentHeight);
    persistentPanel.setBounds (0, HEIGHT - persistentHeight, WIDTH, persistentHeight);

    // Layout persistent panel
    auto panelArea = persistentPanel.getLocalBounds();

    // Bottom control row (below keyboard)
    constexpr int controlRowHeight = 28;
    auto controlRow = panelArea.removeFromBottom (controlRowHeight);

    constexpr int controlItemWidth = 130;
    pitchBendRangeControl.setBounds (controlRow.removeFromLeft (controlItemWidth).reduced (2, 2));
    voiceCountControl.setBounds (controlRow.removeFromLeft (controlItemWidth).reduced (2, 2));
    volumeControl.setBounds (controlRow.removeFromLeft (controlItemWidth).reduced (2, 2));
    portamentoBottomControl.setBounds (controlRow.removeFromLeft (controlItemWidth).reduced (2, 2));

    constexpr int panicButtonWidth = 80;
    panicButton.setBounds (controlRow.removeFromRight (panicButtonWidth).reduced (2, 2));

    // Keyboard row
    constexpr int keyboardRowHeight = 160;
    auto keyboardRow = panelArea.removeFromBottom (keyboardRowHeight);

    // MW/AT/EXP tabs + wheels box
    constexpr int mwTabColumnWidth = 36;
    int wheelWidth = juce::jmax (36, keyboardRow.getHeight() / 3);
    int wheelBoxWidth = mwTabColumnWidth + wheelWidth * 2 + 4;
    auto wheelBoxArea = keyboardRow.removeFromLeft (wheelBoxWidth);

    // Draw box background behind the whole group
    wheelBoxBackground.setBounds (wheelBoxArea.reduced (Style::componentGap));

    // Inset children by the same padding as ADSR (4 + 4 = 8px per side)
    auto wheelBoxInner = wheelBoxArea.reduced (8);

    // MW/AT/EXP tabs stacked vertically to the left of the mod wheel
    auto mwTabArea = wheelBoxInner.removeFromLeft (mwTabColumnWidth - 4);
    int mwTabH = mwTabArea.getHeight() / 3;
    mwTab.setBounds (mwTabArea.removeFromTop (mwTabH).reduced (1, 2));
    atTab.setBounds (mwTabArea.removeFromTop (mwTabH).reduced (1, 2));
    expTab.setBounds (mwTabArea.reduced (1, 2));

    auto wheelsArea = wheelBoxInner;
    modWheel.setBounds (wheelsArea.removeFromLeft (wheelsArea.getWidth() / 2).reduced (2));
    pitchWheel.setBounds (wheelsArea.reduced (2));

    keyboard.setBounds (keyboardRow.reduced (2));

    // Top: LFO/ADSR/KeyVel row — tabs integrated into component boxes
    auto row3 = panelArea;
    constexpr int tabButtonHeight = 26;
    constexpr int tabBottomPad = 8;  // match outer-to-inner box padding
    constexpr int innerTabHeight = tabButtonHeight + tabBottomPad;

    int contentHalf = row3.getWidth() / 2;
    int adsrWidth = row3.getWidth() / 3;

    // Set component bounds
    auto lfoBounds = row3.removeFromLeft (contentHalf).reduced (Style::componentGap);
    auto adsrBounds = row3.removeFromLeft (adsrWidth).reduced (Style::componentGap);
    auto kvBounds = row3.reduced (Style::componentGap);

    lfoComponent.setBottomTabReserve (innerTabHeight);
    adsrGraph.setBottomTabReserve (innerTabHeight);
    keyVelComponent.setBottomTabReserve (innerTabHeight);

    lfoComponent.setBounds (lfoBounds);
    adsrGraph.setBounds (adsrBounds);
    keyVelComponent.setBounds (kvBounds);

    // LFO tabs at bottom of LFO outer box
    {
        auto tabStrip = lfoBounds.removeFromBottom (innerTabHeight);
        tabStrip.removeFromBottom (tabBottomPad);
        tabStrip = tabStrip.reduced (8, 0);
        int tabW = tabStrip.getWidth() / 4;
        for (int i = 0; i < 4; ++i)
            lfoTabs[i].setBounds (tabStrip.removeFromLeft (tabW).reduced (1, 0));
    }

    // ENV tabs at bottom of ADSR outer box
    {
        auto tabStrip = adsrBounds.removeFromBottom (innerTabHeight);
        tabStrip.removeFromBottom (tabBottomPad);
        tabStrip = tabStrip.reduced (8, 0);
        int tabW = tabStrip.getWidth() / 4;
        for (int i = 0; i < 4; ++i)
            envTabs[i].setBounds (tabStrip.removeFromLeft (tabW).reduced (1, 0));
    }

    // Vel/Key tabs at bottom of KeyVel outer box
    {
        auto tabStrip = kvBounds.removeFromBottom (innerTabHeight);
        tabStrip.removeFromBottom (tabBottomPad);
        tabStrip = tabStrip.reduced (8, 0);
        int tabW = tabStrip.getWidth() / 2;
        velTab.setBounds (tabStrip.removeFromLeft (tabW).reduced (1, 0));
        keyTab.setBounds (tabStrip.reduced (1, 0));
    }

    // Position mode label to the right of the tab bar
    auto& tabBar = tabbedComponent.getTabbedButtonBar();
    int tabBarRight = 0;
    for (int i = 0; i < tabBar.getNumTabs(); ++i)
    {
        if (auto* btn = tabBar.getTabButton (i))
            tabBarRight = juce::jmax (tabBarRight, btn->getRight());
    }
    auto tabBarBounds = tabBar.getBounds();
    // Preset navigation group: same width as one tab column (1/6)
    int presetGroupWidth = WIDTH / 6;
    int presetGroupY = tabBarBounds.getY() + 2;
    int presetGroupH = tabBarBounds.getHeight() - 4;
    int arrowWidth = presetGroupH;
    int nameWidth = presetGroupWidth - 2 * arrowWidth;

    prevPresetButton.setBounds (tabBarRight, presetGroupY, arrowWidth, presetGroupH);
    presetButton.setBounds (tabBarRight + arrowWidth, presetGroupY, nameWidth, presetGroupH);
    nextPresetButton.setBounds (tabBarRight + arrowWidth + nameWidth, presetGroupY, arrowWidth, presetGroupH);


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

void PluginEditor::targetSourceChanged (const juce::String& newSourceID)
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
