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
      adsrGraph (p.parameters, "env1Attack", "env1AttackCurve", "env1Decay", "env1DecayCurve", "env1Sustain", "env1Release", "env1ReleaseCurve", p.getSynth().getAmpADSRPtr(), &p.undoManager, &p.activeGestureCount)
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

void MainTabContent::modulationModeChanged (ModulationModeState::Mode)
{
    for (int i = 0; i < 4; ++i)
    {
        lfoTabs[i].repaint();
        envTabs[i].repaint();
    }
    repaint();
}

void MainTabContent::targetSourceChanged (const juce::String&)
{
    for (int i = 0; i < 4; ++i)
    {
        lfoTabs[i].repaint();
        envTabs[i].repaint();
    }
}

void MainTabContent::paint (juce::Graphics& g)
{
    g.fillAll (PRIMARY_COLOR);
}

void MainTabContent::resized()
{
    auto area = getLocalBounds();
    auto rowHeight = area.getHeight() / 3;

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

    // Row 3: 30px tab strip at top, then LFO | ADSR below
    auto row3 = area;
    auto buttonRow = row3.removeFromTop (30);
    auto lfoTabArea = buttonRow.removeFromLeft (buttonRow.getWidth() / 2);
    auto envTabArea = buttonRow;

    // LFO tabs
    int lfoTabWidth = juce::jmin (75, lfoTabArea.getWidth() / 4);
    for (int i = 0; i < 4; ++i)
        lfoTabs[i].setBounds (lfoTabArea.removeFromLeft (lfoTabWidth));

    // ENV tabs
    int envTabWidth = juce::jmin (75, envTabArea.getWidth() / 4);
    for (int i = 0; i < 4; ++i)
        envTabs[i].setBounds (envTabArea.removeFromLeft (envTabWidth));

    // LFO component | ADSR graph
    lfoComponent.setBounds (row3.removeFromLeft (row3.getWidth() / 2).reduced (5));
    adsrGraph.setBounds (row3.reduced (5));
}

// =============================================================================
// OscillatorTabContent
// =============================================================================

OscillatorTabContent::OscillatorTabContent (PluginProcessor& p)
    : waveformComponent (p.parameters, &p.undoManager, &p.activeGestureCount, &p.modDestOutputs[4], &p.modDestOutputs[5]),
      detuneComponent (p.parameters, &p.undoManager, &p.activeGestureCount, &p.modDestOutputs[6], &p.modDestOutputs[7]),
      subOscillatorComponent (p.parameters, &p.undoManager, &p.activeGestureCount, &p.modDestOutputs[8], &p.modDestOutputs[9]),
      oscilloscope (p)
{
    addAndMakeVisible (waveformComponent);
    addAndMakeVisible (detuneComponent);
    addAndMakeVisible (subOscillatorComponent);
    addAndMakeVisible (oscilloscope);
}

void OscillatorTabContent::paint (juce::Graphics& g)
{
    g.fillAll (PRIMARY_COLOR);
}

void OscillatorTabContent::resized()
{
    auto area = getLocalBounds();

    // Left column: waveform, detune, sub oscillator
    auto leftColumn = area.removeFromLeft (area.getWidth() / 4);
    auto cellHeight = leftColumn.getHeight() / 3;
    waveformComponent.setBounds (leftColumn.removeFromTop (cellHeight).reduced (5));
    detuneComponent.setBounds (leftColumn.removeFromTop (cellHeight).reduced (5));
    subOscillatorComponent.setBounds (leftColumn.reduced (5));

    // Right: oscilloscope takes remaining space
    oscilloscope.setBounds (area.reduced (5));
}

// =============================================================================
// ModulationTabContent
// =============================================================================

ModulationTabContent::ModulationTabContent (PluginProcessor& p)
    : matrixComponent (p.modTree, p.modSourceOutputs.data())
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
}

void ExperimentTabContent::paint (juce::Graphics& g)
{
    g.fillAll (PRIMARY_COLOR);

    g.setColour (TEXT_COLOR);
    g.setFont (18.0f);
    g.drawText ("Voice Mode", getLocalBounds().removeFromTop (40), juce::Justification::centred);
}

void ExperimentTabContent::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (50);

    auto toggleArea = area.removeFromTop (60).withSizeKeepingCentre (200, 60);
    monoToggle.setBounds (toggleArea.removeFromTop (30));
    legatoToggle.setBounds (toggleArea.removeFromTop (30));
}
