#pragma once

#include "../../PluginProcessor.h"
#include "../../Theme.h"
#include "../ADSRGraph.h"
#include "../ChorusComponent.h"
#include "../DetuneComponent.h"
#include "../FilterDisplay.h"
#include "../LFOComponent.h"
#include "../MatrixComponent.h"
#include "../OscGraph.h"
#include "../Oscilliscope.h"
#include "../SubOscillatorComponent.h"
#include "../Utility/SingleParameterComponent.h"

// =============================================================================
// Main Tab: Waveform, Filter Display, ADSR Graph, Oscilloscope, Volume
// =============================================================================
class MainTabContent final : public juce::Component
{
public:
    explicit MainTabContent (PluginProcessor& p);
    void resized() override;
    void paint (juce::Graphics& g) override;

private:
    WaveformComponent waveformComponent;
    FilterDisplay filterDisplay;
    ADSRGraph adsrGraph;
    Oscilloscope oscilloscope;
    SingleParameterComponent volumeComponent;
    LFOComponent lfoComponent;
};

// =============================================================================
// Oscillator Tab: Waveform, Detune, Sub Oscillator, Oscilloscope
// =============================================================================
class OscillatorTabContent final : public juce::Component
{
public:
    explicit OscillatorTabContent (PluginProcessor& p);
    void resized() override;
    void paint (juce::Graphics& g) override;

private:
    WaveformComponent waveformComponent;
    DetuneComponent detuneComponent;
    SubOscillatorComponent subOscillatorComponent;
    Oscilloscope oscilloscope;
};

// =============================================================================
// Modulation Tab: LFO (with rate slider), ADSR Graph, Modulation Matrix
// =============================================================================
class ModulationTabContent final : public juce::Component
{
public:
    explicit ModulationTabContent (PluginProcessor& p);
    void resized() override;
    void paint (juce::Graphics& g) override;

private:
    LFOComponent lfoComponent;
    ADSRGraph adsrGraph;
    MatrixComponent matrixComponent;
};

// =============================================================================
// Effects Tab: Filter Display, Chorus, Volume, Chorus Mix
// =============================================================================
class EffectsTabContent final : public juce::Component
{
public:
    explicit EffectsTabContent (PluginProcessor& p);
    void resized() override;
    void paint (juce::Graphics& g) override;

private:
    FilterDisplay filterDisplay;
    ChorusComponent chorusComponent;
    SingleParameterComponent volumeComponent;
    SingleParameterComponent chorusMixComponent;
};
