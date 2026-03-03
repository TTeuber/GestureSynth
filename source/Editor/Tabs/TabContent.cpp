#include "TabContent.h"

// =============================================================================
// MainTabContent
// =============================================================================

MainTabContent::MainTabContent (PluginProcessor& p, ModulationModeState* modState)
    : processor (p),
      modModeState (modState),
      waveformComponent (p.parameters, &p.undoManager, &p.activeGestureCount, &p.modDestOutputs[4], &p.modDestOutputs[5], modState, "oscWaveform", "pulseWidth"),
      filterDisplay (p.parameters, &p.undoManager, &p.activeGestureCount, &p.modDestOutputs[0], &p.modDestOutputs[1], modState),
      hpfDisplay (p.parameters, &p.undoManager, &p.activeGestureCount, modState),
      subOscillatorComponent (p.parameters, &p.undoManager, &p.activeGestureCount, &p.modDestOutputs[8], &p.modDestOutputs[9], modState, "subOsc", "subOscWave"),
      detuneComponent (p.parameters, &p.undoManager, &p.activeGestureCount, &p.modDestOutputs[6], &p.modDestOutputs[7], modState, "oscDetune", "oscWidth"),
      chorusComponent (p.parameters, &p.undoManager, &p.activeGestureCount, &p.modDestOutputs[12], &p.modDestOutputs[13], modState, "chorusDepth", "chorusRate"),
      vibratoComponent (p.parameters, &p.undoManager, &p.activeGestureCount, &p.modDestOutputs[10], &p.modDestOutputs[11], modState, "vibratoDepth", "vibratoRate"),
      volumeComponent (p.parameters.getParameter ("volume"), &p.undoManager, &p.activeGestureCount, modState),
      noiseComponent (p.parameters.getParameter ("noiseLevel"), &p.undoManager, &p.activeGestureCount, modState),
      chorusMixComponent (p.parameters.getParameter ("chorusMix"), &p.undoManager, &p.activeGestureCount, modState),
      portamentoComponent (p.parameters.getParameter ("portamentoTime"), &p.undoManager, &p.activeGestureCount, modState),
      lfoComponent (p.lfoData[0], p.parameters, true, 1),
      adsrGraph (p.parameters, "env1Attack", "env1AttackCurve", "env1Decay", "env1DecayCurve", "env1Sustain", "env1Release", "env1ReleaseCurve", p.getSynth().getAmpADSRPtr(), &p.undoManager, &p.activeGestureCount),
      keyVelComponent (p.parameters, &p.undoManager, &p.activeGestureCount, modState, p.getSynth().getVelocityRawPtr(), p.getSynth().getKeyboardRawPtr()),
      modWheel (p, modState),
      pitchWheel (p),
      keyboard (p.keyboardState),
      pitchBendRangeControl (p.parameters.getParameter ("pitchBendRange")),
      voiceCountControl (dynamic_cast<juce::AudioParameterChoice*> (p.parameters.getParameter ("voiceCount"))),
      monoToggle (dynamic_cast<juce::AudioParameterBool*> (p.parameters.getParameter ("monoOn")), "Mono"),
      legatoToggle (dynamic_cast<juce::AudioParameterBool*> (p.parameters.getParameter ("legatoOn")), "Legato"),
      gateToggle (dynamic_cast<juce::AudioParameterBool*> (p.parameters.getParameter ("gateMode")), "Gate")
{
    addAndMakeVisible (waveformComponent);
    addAndMakeVisible (filterDisplay);
    addAndMakeVisible (hpfDisplay);
    addAndMakeVisible (subOscillatorComponent);
    addAndMakeVisible (detuneComponent);
    addAndMakeVisible (chorusComponent);
    addAndMakeVisible (vibratoComponent);
    addAndMakeVisible (volumeComponent);
    addAndMakeVisible (noiseComponent);
    addAndMakeVisible (chorusMixComponent);
    addAndMakeVisible (portamentoComponent);
    addAndMakeVisible (lfoComponent);
    addAndMakeVisible (adsrGraph);
    addAndMakeVisible (keyVelComponent);
    addAndMakeVisible (modWheel);
    addAndMakeVisible (pitchWheel);
    addAndMakeVisible (keyboard);
    addAndMakeVisible (pitchBendRangeControl);
    addAndMakeVisible (voiceCountControl);
    addAndMakeVisible (monoToggle);
    addAndMakeVisible (legatoToggle);
    addAndMakeVisible (gateToggle);

    // MW / AT / EXP tabs — select only sets mod target source, no content panel
    mwTab.setup ("MW", "modWheel", modModeState, [this]
    {
        if (modModeState != nullptr)
        {
            modModeState->setTargetSource ("modWheel");
            if (!modModeState->isModulationMode())
                modModeState->setMode (ModulationModeState::Mode::Modulation);
        }
    });
    atTab.setup ("AT", "aftertouch", modModeState, [this]
    {
        if (modModeState != nullptr)
        {
            modModeState->setTargetSource ("aftertouch");
            if (!modModeState->isModulationMode())
                modModeState->setMode (ModulationModeState::Mode::Modulation);
        }
    });
    expTab.setup ("EXP", "expression", modModeState, [this]
    {
        if (modModeState != nullptr)
        {
            modModeState->setTargetSource ("expression");
            if (!modModeState->isModulationMode())
                modModeState->setMode (ModulationModeState::Mode::Modulation);
        }
    });
    mwTab.setCompactMode (true);
    atTab.setCompactMode (true);
    expTab.setCompactMode (true);
    addAndMakeVisible (mwTab);
    addAndMakeVisible (atTab);
    addAndMakeVisible (expTab);

    const juce::StringArray lfoIDs = { "lfo1", "lfo2", "lfo3", "lfo4" };
    const juce::StringArray envIDs = { "env1", "env2", "env3", "env4" };

    for (int i = 0; i < 4; ++i)
    {
        lfoTabs[i].setup ("LFO " + juce::String (i + 1), lfoIDs[i], modModeState,
            [this, i]() { selectLfo (i); });
        addAndMakeVisible (lfoTabs[i]);

        envTabs[i].setup ("ENV " + juce::String (i + 1), envIDs[i], modModeState,
            [this, i]() { selectEnv (i); });
        addAndMakeVisible (envTabs[i]);
    }

    lfoTabs[0].setSelected (true);
    envTabs[0].setSelected (true);

    velTab.setup ("Vel", "velocity", modModeState, [this] { selectKeyVel (0); });
    keyTab.setup ("Key", "keyboard", modModeState, [this] { selectKeyVel (1); });
    addAndMakeVisible (velTab);
    addAndMakeVisible (keyTab);
    velTab.setSelected (true);

    if (modModeState != nullptr)
        modModeState->addListener (this);
}

MainTabContent::~MainTabContent()
{
    if (modModeState != nullptr)
        modModeState->removeListener (this);
}

void MainTabContent::selectLfo (int index)
{
    activeLfoIndex = index;
    for (int i = 0; i < 4; ++i)
        lfoTabs[i].setSelected (i == index);
    lfoComponent.rebind (processor.lfoData[index], index + 1);
}

void MainTabContent::selectEnv (int index)
{
    activeEnvIndex = index;
    for (int i = 0; i < 4; ++i)
        envTabs[i].setSelected (i == index);
    auto s = std::to_string (index + 1);
    auto adsrPtr = (index == 0) ? processor.getSynth().getAmpADSRPtr() : std::make_shared<MyADSR*> (nullptr);
    adsrGraph.rebind (
        "env" + s + "Attack",
        "env" + s + "AttackCurve",
        "env" + s + "Decay",
        "env" + s + "DecayCurve",
        "env" + s + "Sustain",
        "env" + s + "Release",
        "env" + s + "ReleaseCurve",
        adsrPtr);
}

void MainTabContent::selectKeyVel (int index)
{
    activeKeyVelTab = index;
    velTab.setSelected (index == 0);
    keyTab.setSelected (index == 1);
    keyVelComponent.setActiveTab (index);
}

void MainTabContent::modulationModeChanged (ModulationModeState::Mode)
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
    repaint();
}

void MainTabContent::targetSourceChanged (const juce::String&)
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
    hpfDisplay.repaint();
    filterDisplay.repaint();
}

void MainTabContent::paint (juce::Graphics& g)
{
    g.fillAll (PRIMARY_COLOR);
}

void MainTabContent::resized()
{
    auto area = getLocalBounds();
    auto bottomHeight = area.getHeight() * 22 / 100;
    auto bottomRow = area.removeFromBottom (bottomHeight);

    // Bottom row: ModWheel | PitchWheel | Keyboard + controls | [Vel/Key tabs + KeyVel]
    int wheelWidth = juce::jmax (36, bottomRow.getHeight() / 3);
    auto wheelsArea = bottomRow.removeFromLeft (wheelWidth * 2 + 4);
    modWheel.setBounds (wheelsArea.removeFromLeft (wheelWidth).reduced (2));
    pitchWheel.setBounds (wheelsArea.removeFromLeft (wheelWidth).reduced (2));

    // KeyVel column on the right (square aspect ratio)
    int kvWidth = bottomRow.getHeight() + 10;
    auto kvColumn = bottomRow.removeFromRight (kvWidth);
    keyVelComponent.setBounds (kvColumn.reduced (5));

    // Control row below keyboard
    constexpr int controlRowHeight = 28;
    auto controlRow = bottomRow.removeFromBottom (controlRowHeight);

    // Layout: [PitchBend] [Voices] [Mono] [Legato] [Gate]
    int controlWidth = controlRow.getWidth();
    int pbWidth = controlWidth * 25 / 100;
    int vcWidth = controlWidth * 20 / 100;
    int toggleWidth = (controlWidth - pbWidth - vcWidth) / 3;

    pitchBendRangeControl.setBounds (controlRow.removeFromLeft (pbWidth).reduced (2, 2));
    voiceCountControl.setBounds (controlRow.removeFromLeft (vcWidth).reduced (2, 2));
    monoToggle.setBounds (controlRow.removeFromLeft (toggleWidth).reduced (4, 3));
    legatoToggle.setBounds (controlRow.removeFromLeft (toggleWidth).reduced (4, 3));
    gateToggle.setBounds (controlRow.reduced (4, 3));

    // Keyboard fills remaining space above controls
    keyboard.setBounds (bottomRow.reduced (2));

    auto rowHeight = area.getHeight() * 3 / 10;
    auto squareCol = rowHeight;

    // Row 1: 2x2 grid | Chorus | Vibrato | FilterDisplay (fills remaining)
    auto row1 = area.removeFromTop (rowHeight);
    auto mixCol = row1.removeFromLeft (squareCol);
    auto topRow = mixCol.removeFromTop (mixCol.getHeight() / 2);
    volumeComponent.setBounds (topRow.removeFromLeft (topRow.getWidth() / 2).reduced (5));
    noiseComponent.setBounds (topRow.reduced (5));
    chorusMixComponent.setBounds (mixCol.removeFromLeft (mixCol.getWidth() / 2).reduced (5));
    portamentoComponent.setBounds (mixCol.reduced (5));
    chorusComponent.setBounds (row1.removeFromLeft (squareCol).reduced (5));
    vibratoComponent.setBounds (row1.removeFromLeft (squareCol).reduced (5));
    filterDisplay.setBounds (row1.reduced (5));

    // Row 2: SubOsc | Waveform | Detune | HPF (fills remaining)
    auto row2 = area.removeFromTop (rowHeight);
    subOscillatorComponent.setBounds (row2.removeFromLeft (squareCol).reduced (5));
    waveformComponent.setBounds (row2.removeFromLeft (squareCol).reduced (5));
    detuneComponent.setBounds (row2.removeFromLeft (squareCol).reduced (5));
    hpfDisplay.setBounds (row2.reduced (5));

    // Row 3: LFO | ADSR (split full width), 30px tab strip at bottom
    auto row3 = area;
    auto buttonRow = row3.removeFromBottom (30).reduced (5, 0);

    int contentHalf = row3.getWidth() / 2;
    lfoComponent.setBounds (row3.removeFromLeft (contentHalf).reduced (5));
    adsrGraph.setBounds (row3.reduced (5));

    // Tab strip: MW, AT, EXP (narrow) | LFO 1-4 | ENV 1-4 | Vel, Key (narrow)
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
}

// =============================================================================
// KeyboardTabContent
// =============================================================================

KeyboardTabContent::KeyboardTabContent (PluginProcessor& p)
    : oscilloscope (p),
      keyboard (p.keyboardState)
{
    addAndMakeVisible (oscilloscope);
    addAndMakeVisible (keyboard);
}

void KeyboardTabContent::paint (juce::Graphics& g)
{
    g.fillAll (PRIMARY_COLOR);
}

void KeyboardTabContent::resized()
{
    auto area = getLocalBounds();

    // Top 60%: oscilloscope
    auto oscArea = area.removeFromTop (static_cast<int> (area.getHeight() * 0.6f));
    oscilloscope.setBounds (oscArea.reduced (5));

    // Bottom 40%: custom keyboard
    keyboard.setBounds (area.reduced (2));
}

// =============================================================================
// ModulationTabContent
// =============================================================================

ModulationTabContent::ModulationTabContent (PluginProcessor& p)
    : matrixComponent (p.modTree, p.modSourceOutputs.data(), &p.undoManager, &p.activeGestureCount)
{
    addAndMakeVisible (matrixComponent);
}

void ModulationTabContent::paint (juce::Graphics& g)
{
    g.fillAll (PRIMARY_COLOR);
}

void ModulationTabContent::resized()
{
    matrixComponent.setBounds (getLocalBounds().reduced (5));
}

// =============================================================================
// EffectsTabContent
// =============================================================================

EffectsTabContent::EffectsTabContent (PluginProcessor& p)
    : filterDisplay (p.parameters, &p.undoManager, &p.activeGestureCount, &p.modDestOutputs[0], &p.modDestOutputs[1]),
      chorusComponent (p.parameters, &p.undoManager, &p.activeGestureCount, &p.modDestOutputs[12], &p.modDestOutputs[13]),
      vibratoComponent (p.parameters, &p.undoManager, &p.activeGestureCount, &p.modDestOutputs[10], &p.modDestOutputs[11]),
      volumeComponent (p.parameters.getParameter ("volume"), &p.undoManager, &p.activeGestureCount),
      noiseComponent (p.parameters.getParameter ("noiseLevel"), &p.undoManager, &p.activeGestureCount),
      chorusMixComponent (p.parameters.getParameter ("chorusMix"), &p.undoManager, &p.activeGestureCount),
      portamentoComponent (p.parameters.getParameter ("portamentoTime"), &p.undoManager, &p.activeGestureCount)
{
    addAndMakeVisible (filterDisplay);
    addAndMakeVisible (chorusComponent);
    addAndMakeVisible (vibratoComponent);
    addAndMakeVisible (volumeComponent);
    addAndMakeVisible (noiseComponent);
    addAndMakeVisible (chorusMixComponent);
    addAndMakeVisible (portamentoComponent);
}

void EffectsTabContent::paint (juce::Graphics& g)
{
    g.fillAll (PRIMARY_COLOR);
}

void EffectsTabContent::resized()
{
    auto area = getLocalBounds();

    // Top half: filter display
    auto topHalf = area.removeFromTop (area.getHeight() / 2);
    filterDisplay.setBounds (topHalf.reduced (5));

    // Bottom half: 3 columns — chorus | vibrato | volume+chorusMix
    auto colWidth = area.getWidth() / 3;
    auto chorusArea = area.removeFromLeft (colWidth);
    auto vibratoArea = area.removeFromLeft (colWidth);
    auto rightColumn = area;

    chorusComponent.setBounds (chorusArea.reduced (5));
    vibratoComponent.setBounds (vibratoArea.reduced (5));
    auto topRight = rightColumn.removeFromTop (rightColumn.getHeight() / 2);
    volumeComponent.setBounds (topRight.removeFromLeft (topRight.getWidth() / 2).reduced (5));
    noiseComponent.setBounds (topRight.reduced (5));
    chorusMixComponent.setBounds (rightColumn.removeFromLeft (rightColumn.getWidth() / 2).reduced (5));
    portamentoComponent.setBounds (rightColumn.reduced (5));
}

// =============================================================================
// ExperimentTabContent
// =============================================================================

ExperimentTabContent::ExperimentTabContent (PluginProcessor& p)
{
    monoToggle.setColour (juce::ToggleButton::textColourId, TEXT_COLOR);
    monoToggle.setColour (juce::ToggleButton::tickColourId, TEXT_COLOR);
    addAndMakeVisible (monoToggle);
    monoAttachment = std::make_unique<ButtonAttachment> (p.parameters, "monoOn", monoToggle);

    legatoToggle.setColour (juce::ToggleButton::textColourId, TEXT_COLOR);
    legatoToggle.setColour (juce::ToggleButton::tickColourId, TEXT_COLOR);
    addAndMakeVisible (legatoToggle);
    legatoAttachment = std::make_unique<ButtonAttachment> (p.parameters, "legatoOn", legatoToggle);

    gateModeToggle.setColour (juce::ToggleButton::textColourId, TEXT_COLOR);
    gateModeToggle.setColour (juce::ToggleButton::tickColourId, TEXT_COLOR);
    addAndMakeVisible (gateModeToggle);
    gateModeAttachment = std::make_unique<ButtonAttachment> (p.parameters, "gateMode", gateModeToggle);

    pitchBendRangeSlider.setSliderStyle (juce::Slider::IncDecButtons);
    pitchBendRangeSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 40, 24);
    pitchBendRangeSlider.setColour (juce::Slider::textBoxTextColourId, TEXT_COLOR);
    pitchBendRangeSlider.setColour (juce::Slider::textBoxOutlineColourId, SECONDARY_COLOR);
    addAndMakeVisible (pitchBendRangeSlider);
    pitchBendRangeAttachment = std::make_unique<SliderAttachment> (p.parameters, "pitchBendRange", pitchBendRangeSlider);

    pitchBendRangeLabel.setColour (juce::Label::textColourId, TEXT_COLOR);
    pitchBendRangeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (pitchBendRangeLabel);

    manualBpmSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    manualBpmSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 24);
    manualBpmSlider.setColour (juce::Slider::textBoxTextColourId, TEXT_COLOR);
    manualBpmSlider.setColour (juce::Slider::textBoxBackgroundColourId, SECONDARY_COLOR);
    manualBpmSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    manualBpmSlider.setTextValueSuffix (" BPM");
    addAndMakeVisible (manualBpmSlider);
    manualBpmAttachment = std::make_unique<SliderAttachment> (p.parameters, "manualBpm", manualBpmSlider);

    manualBpmLabel.setColour (juce::Label::textColourId, TEXT_COLOR);
    manualBpmLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (manualBpmLabel);

    // Reverb sliders
    auto setupReverbSlider = [this] (juce::Slider& slider)
    {
        slider.setSliderStyle (juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 20);
        slider.setColour (juce::Slider::textBoxTextColourId, TEXT_COLOR);
        slider.setColour (juce::Slider::textBoxBackgroundColourId, SECONDARY_COLOR);
        slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (slider);
    };

    setupReverbSlider (reverbDecaySlider);
    setupReverbSlider (reverbSizeSlider);
    setupReverbSlider (reverbDampingSlider);
    setupReverbSlider (reverbBassMultSlider);
    setupReverbSlider (reverbModRateSlider);
    setupReverbSlider (reverbModDepthSlider);
    setupReverbSlider (reverbDiffusionSlider);
    setupReverbSlider (reverbPreDelaySlider);
    setupReverbSlider (reverbWidthSlider);
    setupReverbSlider (reverbMixSlider);

    reverbDecayAttachment     = std::make_unique<SliderAttachment> (p.parameters, "reverbDecay", reverbDecaySlider);
    reverbSizeAttachment      = std::make_unique<SliderAttachment> (p.parameters, "reverbSize", reverbSizeSlider);
    reverbDampingAttachment   = std::make_unique<SliderAttachment> (p.parameters, "reverbDamping", reverbDampingSlider);
    reverbBassMultAttachment  = std::make_unique<SliderAttachment> (p.parameters, "reverbBassMult", reverbBassMultSlider);
    reverbModRateAttachment   = std::make_unique<SliderAttachment> (p.parameters, "reverbModRate", reverbModRateSlider);
    reverbModDepthAttachment  = std::make_unique<SliderAttachment> (p.parameters, "reverbModDepth", reverbModDepthSlider);
    reverbDiffusionAttachment = std::make_unique<SliderAttachment> (p.parameters, "reverbDiffusion", reverbDiffusionSlider);
    reverbPreDelayAttachment  = std::make_unique<SliderAttachment> (p.parameters, "reverbPreDelay", reverbPreDelaySlider);
    reverbWidthAttachment     = std::make_unique<SliderAttachment> (p.parameters, "reverbWidth", reverbWidthSlider);
    reverbMixAttachment       = std::make_unique<SliderAttachment> (p.parameters, "reverbMix", reverbMixSlider);
}

void ExperimentTabContent::paint (juce::Graphics& g)
{
    g.fillAll (PRIMARY_COLOR);

    g.setColour (TEXT_COLOR);
    g.setFont (18.0f);
    g.drawText ("Voice Mode", getLocalBounds().removeFromTop (40), juce::Justification::centred);
    g.drawText ("Pitch Bend", getLocalBounds().withTop (120).removeFromTop (30), juce::Justification::centred);
    g.drawText ("Manual BPM", getLocalBounds().withTop (210).removeFromTop (30), juce::Justification::centred);

    // Reverb section header
    g.setFont (18.0f);
    g.drawText ("Reverb", getLocalBounds().withTop (280).removeFromTop (30), juce::Justification::centred);

    // Reverb slider labels (left column)
    g.setFont (12.0f);
    int reverbTop = 310;
    int sliderH = 24;
    int spacing = 4;
    int labelW = 80;
    int leftX = getWidth() / 2 - 240;
    int rightX = getWidth() / 2 + 10;

    const char* leftLabels[] = { "Decay", "Size", "Damping", "Bass Mult", "Mod Rate" };
    const char* rightLabels[] = { "Mod Depth", "Diffusion", "Pre-Delay", "Width", "Mix" };

    for (int i = 0; i < 5; ++i)
    {
        int y = reverbTop + i * (sliderH + spacing);
        g.drawText (leftLabels[i], leftX, y, labelW, sliderH, juce::Justification::centredRight);
        g.drawText (rightLabels[i], rightX, y, labelW, sliderH, juce::Justification::centredRight);
    }
}

void ExperimentTabContent::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (50);

    auto toggleArea = area.removeFromTop (90).withSizeKeepingCentre (200, 90);
    monoToggle.setBounds (toggleArea.removeFromTop (30));
    legatoToggle.setBounds (toggleArea.removeFromTop (30));
    gateModeToggle.setBounds (toggleArea.removeFromTop (30));

    area.removeFromTop (30); // space for "Pitch Bend" header
    auto sliderArea = area.removeFromTop (30).withSizeKeepingCentre (200, 30);
    pitchBendRangeLabel.setBounds (sliderArea.removeFromTop (0)); // label is drawn via paint()
    pitchBendRangeSlider.setBounds (sliderArea);

    area.removeFromTop (30); // space for "Manual BPM" header
    auto bpmArea = area.removeFromTop (30).withSizeKeepingCentre (250, 30);
    manualBpmSlider.setBounds (bpmArea);

    // Reverb sliders: 2 columns of 5, below existing controls
    int reverbTop = 310;
    int sliderH = 24;
    int spacing = 4;
    int labelW = 80;
    int sliderW = 150;
    int leftX = getWidth() / 2 - 240 + labelW + 5;
    int rightX = getWidth() / 2 + 10 + labelW + 5;

    juce::Slider* leftSliders[] = { &reverbDecaySlider, &reverbSizeSlider, &reverbDampingSlider, &reverbBassMultSlider, &reverbModRateSlider };
    juce::Slider* rightSliders[] = { &reverbModDepthSlider, &reverbDiffusionSlider, &reverbPreDelaySlider, &reverbWidthSlider, &reverbMixSlider };

    for (int i = 0; i < 5; ++i)
    {
        int y = reverbTop + i * (sliderH + spacing);
        leftSliders[i]->setBounds (leftX, y, sliderW, sliderH);
        rightSliders[i]->setBounds (rightX, y, sliderW, sliderH);
    }
}
