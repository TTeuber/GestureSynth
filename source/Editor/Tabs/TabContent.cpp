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
      lfoComponent (p.lfoData, p.parameters, false)
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
    : lfoComponent (p.lfoData, p.parameters, true),
      adsrGraph (p.parameters, "env1Attack", "env1AttackCurve", "env1Decay", "env1DecayCurve", "env1Sustain", "env1Release", "env1ReleaseCurve", p.getSynth().getAmpADSRPtr()),
      matrixComponent (p.modTree)
{
    addAndMakeVisible (lfoComponent);
    addAndMakeVisible (adsrGraph);
    addAndMakeVisible (matrixComponent);
}

void ModulationTabContent::paint (juce::Graphics& g)
{
    g.fillAll (PRIMARY_COLOR);
}

void ModulationTabContent::resized()
{
    auto area = getLocalBounds();

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
      volumeComponent (p.parameters.getParameter ("volume")),
      chorusMixComponent (p.parameters.getParameter ("chorusMix"))
{
    addAndMakeVisible (filterDisplay);
    addAndMakeVisible (chorusComponent);
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

    // Bottom half: chorus + dials
    auto bottomLeft = area.removeFromLeft (area.getWidth() / 2);
    chorusComponent.setBounds (bottomLeft.reduced (5));

    // Right side: volume and chorus mix stacked
    volumeComponent.setBounds (area.removeFromTop (area.getHeight() / 2).reduced (5));
    chorusMixComponent.setBounds (area.reduced (5));
}
