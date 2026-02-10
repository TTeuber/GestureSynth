#include "TabContent.h"

// =============================================================================
// MainTabContent
// =============================================================================

MainTabContent::MainTabContent (PluginProcessor& p)
    : waveformComponent (p.parameters),
      filterDisplay (p.parameters),
      adsrGraph (p.parameters, "env1Attack", "env1AttackCurve", "env1Decay", "env1DecayCurve", "env1Sustain", "env1Release", "env1ReleaseCurve", p.getSynth().getAmpADSRPtr()),
      oscilloscope (p),
      volumeComponent (p.parameters.getParameter ("volume")),
      lfoComponent (p.lfoData[0], p.parameters, false, 1)
{
    addAndMakeVisible (waveformComponent);
    addAndMakeVisible (filterDisplay);
    addAndMakeVisible (adsrGraph);
    addAndMakeVisible (oscilloscope);
    addAndMakeVisible (volumeComponent);
    addAndMakeVisible (lfoComponent);
}

void MainTabContent::paint (juce::Graphics& g)
{
    g.fillAll (PRIMARY_COLOR);
}

void MainTabContent::resized()
{
    auto area = getLocalBounds();

    // Left column: waveform + volume stacked
    auto leftColumn = area.removeFromLeft (area.getWidth() / 5);
    waveformComponent.setBounds (leftColumn.removeFromTop (leftColumn.getHeight() / 2).reduced (5));
    volumeComponent.setBounds (leftColumn.reduced (5));

    // Right area: 2x2 grid
    auto topHalf = area.removeFromTop (area.getHeight() / 2);
    adsrGraph.setBounds (topHalf.removeFromLeft (topHalf.getWidth() / 2).reduced (5));
    filterDisplay.setBounds (topHalf.reduced (5));

    oscilloscope.setBounds (area.removeFromLeft (area.getWidth() / 2).reduced (5));
    lfoComponent.setBounds (area.reduced (5));
}

// =============================================================================
// OscillatorTabContent
// =============================================================================

OscillatorTabContent::OscillatorTabContent (PluginProcessor& p)
    : waveformComponent (p.parameters),
      detuneComponent (p.parameters),
      subOscillatorComponent (p.parameters),
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
    : processor (p),
      lfoComponent (p.lfoData[0], p.parameters, true, 1),
      adsrGraph (p.parameters, "env1Attack", "env1AttackCurve", "env1Decay", "env1DecayCurve", "env1Sustain", "env1Release", "env1ReleaseCurve", p.getSynth().getAmpADSRPtr()),
      matrixComponent (p.modTree)
{
    addAndMakeVisible (lfoComponent);
    addAndMakeVisible (adsrGraph);
    addAndMakeVisible (matrixComponent);

    for (int i = 0; i < 4; ++i)
    {
        lfoButtons[i].setButtonText (juce::String (i + 1));
        lfoButtons[i].setClickingTogglesState (true);
        lfoButtons[i].setRadioGroupId (1001);
        lfoButtons[i].setColour (juce::TextButton::buttonOnColourId, juce::Colours::orange);
        lfoButtons[i].onClick = [this, i]() { selectLfo (i); };
        addAndMakeVisible (lfoButtons[i]);

        envButtons[i].setButtonText (juce::String (i + 1));
        envButtons[i].setClickingTogglesState (true);
        envButtons[i].setRadioGroupId (1002);
        envButtons[i].setColour (juce::TextButton::buttonOnColourId, juce::Colours::orange);
        envButtons[i].onClick = [this, i]() { selectEnv (i); };
        addAndMakeVisible (envButtons[i]);
    }

    lfoButtons[0].setToggleState (true, juce::dontSendNotification);
    envButtons[0].setToggleState (true, juce::dontSendNotification);
}

void ModulationTabContent::selectLfo (int index)
{
    activeLfoIndex = index;
    lfoComponent.rebind (processor.lfoData[index], index + 1);
}

void ModulationTabContent::selectEnv (int index)
{
    activeEnvIndex = index;
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

void ModulationTabContent::paint (juce::Graphics& g)
{
    g.fillAll (PRIMARY_COLOR);

    // Draw LFO / ENV labels in the button row
    g.setColour (TEXT_COLOR);
    g.setFont (14.0f);
    auto w = getWidth();
    g.drawText ("LFO", 0, 0, 40, 30, juce::Justification::centred);
    g.drawText ("ENV", w / 2, 0, 40, 30, juce::Justification::centred);
}

void ModulationTabContent::resized()
{
    auto area = getLocalBounds();

    // Button row at the top (30px)
    auto buttonRow = area.removeFromTop (30);
    auto lfoButtonArea = buttonRow.removeFromLeft (buttonRow.getWidth() / 2);
    auto envButtonArea = buttonRow;

    // LFO label + buttons
    auto lfoLabelArea = lfoButtonArea.removeFromLeft (40);
    juce::ignoreUnused (lfoLabelArea); // painted in paint() if desired
    int lfoBtnWidth = juce::jmin (30, lfoButtonArea.getWidth() / 4);
    for (int i = 0; i < 4; ++i)
        lfoButtons[i].setBounds (lfoButtonArea.removeFromLeft (lfoBtnWidth));

    // ENV label + buttons
    auto envLabelArea = envButtonArea.removeFromLeft (40);
    juce::ignoreUnused (envLabelArea);
    int envBtnWidth = juce::jmin (30, envButtonArea.getWidth() / 4);
    for (int i = 0; i < 4; ++i)
        envButtons[i].setBounds (envButtonArea.removeFromLeft (envBtnWidth));

    // Top half: LFO and ADSR side by side
    auto topHalf = area.removeFromTop (area.getHeight() / 2);
    lfoComponent.setBounds (topHalf.removeFromLeft (topHalf.getWidth() / 2).reduced (5));
    adsrGraph.setBounds (topHalf.reduced (5));

    // Bottom half: Matrix
    matrixComponent.setBounds (area.reduced (5));
}

// =============================================================================
// EffectsTabContent
// =============================================================================

EffectsTabContent::EffectsTabContent (PluginProcessor& p)
    : filterDisplay (p.parameters),
      chorusComponent (p.parameters),
      vibratoComponent (p.parameters),
      volumeComponent (p.parameters.getParameter ("volume")),
      chorusMixComponent (p.parameters.getParameter ("chorusMix"))
{
    addAndMakeVisible (filterDisplay);
    addAndMakeVisible (chorusComponent);
    addAndMakeVisible (vibratoComponent);
    addAndMakeVisible (volumeComponent);
    addAndMakeVisible (chorusMixComponent);
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
    volumeComponent.setBounds (rightColumn.removeFromTop (rightColumn.getHeight() / 2).reduced (5));
    chorusMixComponent.setBounds (rightColumn.reduced (5));
}
