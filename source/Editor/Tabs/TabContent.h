#pragma once

#include "../../PluginProcessor.h"
#include "../../Theme.h"
#include "../ADSRGraph.h"
#include "../ChorusComponent.h"
#include "../VibratoComponent.h"
#include "../DetuneComponent.h"
#include "../FilterDisplay.h"
#include "../HPFDisplay.h"
#include "../LFOComponent.h"
#include "../MatrixComponent.h"
#include "../OscGraph.h"
#include "../CustomKeyboard.h"
#include "../WheelComponent.h"
#include "../KeyVelComponent.h"
#include "../Oscilliscope.h"
#include "../SubOscillatorComponent.h"
#include "../ChorusMixComponent.h"
#include "../NoiseComponent.h"
#include "../PortamentoComponent.h"
#include "../Utility/ModulationModeState.h"
#include "../Utility/SingleParameterComponent.h"
#include "../VolumeComponent.h"

// =============================================================================
// ModSourceTab: a tab button for LFO/ENV with crosshair icon
// =============================================================================
class ModSourceTab final : public juce::Component
{
public:
    ModSourceTab() = default;

    void setup (const juce::String& label, const juce::String& srcID,
        ModulationModeState* modState, std::function<void()> onSelect)
    {
        text = label;
        sourceID = srcID;
        modModeState = modState;
        selectCallback = std::move (onSelect);
    }

    void setSelected (bool s) { selected = s; repaint(); }
    bool isSelected() const { return selected; }
    const juce::String& getSourceID() const { return sourceID; }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Background
        if (selected)
            g.setColour (juce::Colours::orange);
        else
            g.setColour (SECONDARY_COLOR);
        g.fillRoundedRectangle (bounds, 3.0f);

        // Crosshair icon on the left
        const float iconSize = 16.0f;
        auto iconArea = bounds.withWidth (iconSize + 8).reduced (4.0f);
        float cx = iconArea.getCentreX();
        float cy = iconArea.getCentreY();
        float r = iconSize * 0.35f;

        bool isTarget = modModeState != nullptr
            && modModeState->isModulationMode()
            && modModeState->getTargetSourceID() == sourceID;

        g.setColour (isTarget ? MOD_COLOR : TEXT_COLOR.withAlpha (0.6f));
        g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 1.5f);
        g.drawLine (cx - r - 2, cy, cx + r + 2, cy, 1.2f);
        g.drawLine (cx, cy - r - 2, cx, cy + r + 2, 1.2f);

        // Text label
        g.setColour (selected ? juce::Colours::black : TEXT_COLOR);
        g.setFont (13.0f);
        g.drawText (text, bounds.withTrimmedLeft (iconSize + 8), juce::Justification::centred, true);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (!getLocalBounds().contains (e.x, e.y))
            return;

        // Check if click was on the crosshair area
        bool clickedCrosshair = e.x < 24;

        if (modModeState != nullptr)
        {
            if (clickedCrosshair && !modModeState->isModulationMode())
            {
                // Click crosshair in normal mode: enter mod mode with this source
                modModeState->setTargetSource (sourceID);
                modModeState->setMode (ModulationModeState::Mode::Modulation);
                if (selectCallback)
                    selectCallback();
                return;
            }
            if (modModeState->isModulationMode())
            {
                // In mod mode: clicking any part changes target and selects tab
                modModeState->setTargetSource (sourceID);
                if (selectCallback)
                    selectCallback();
                return;
            }
        }

        // Normal mode, click on text area: just select tab
        if (selectCallback)
            selectCallback();
    }

private:
    juce::String text;
    juce::String sourceID;
    ModulationModeState* modModeState = nullptr;
    std::function<void()> selectCallback;
    bool selected = false;
};

// =============================================================================
// Main Tab: Filter, Chorus, Vibrato, Waveform, Sub Osc, Detune, HPF, LFO, ADSR
// =============================================================================
class MainTabContent final : public juce::Component, public ModulationModeState::Listener
{
public:
    explicit MainTabContent (PluginProcessor& p, ModulationModeState* modState = nullptr);
    ~MainTabContent() override;
    void resized() override;
    void paint (juce::Graphics& g) override;

    // ModulationModeState::Listener
    void modulationModeChanged (ModulationModeState::Mode newMode) override;
    void targetSourceChanged (const juce::String& newSourceID) override;

private:
    void selectLfo (int index);
    void selectEnv (int index);

    PluginProcessor& processor;
    ModulationModeState* modModeState = nullptr;
    WaveformComponent waveformComponent;
    FilterDisplay filterDisplay;
    HPFDisplay hpfDisplay;
    SubOscillatorComponent subOscillatorComponent;
    DetuneComponent detuneComponent;
    ChorusComponent chorusComponent;
    VibratoComponent vibratoComponent;
    VolumeComponent volumeComponent;
    NoiseComponent noiseComponent;
    ChorusMixComponent chorusMixComponent;
    PortamentoComponent portamentoComponent;
    LFOComponent lfoComponent;
    ADSRGraph adsrGraph;
    KeyVelComponent keyVelComponent;
    ModWheelComponent modWheel;
    PitchWheelComponent pitchWheel;
    CustomKeyboard keyboard;

    void selectKeyVel (int index);

    ModSourceTab mwTab, atTab, expTab;
    ModSourceTab lfoTabs[4];
    ModSourceTab envTabs[4];
    ModSourceTab velTab, keyTab;
    int activeLfoIndex = 0;
    int activeEnvIndex = 0;
    int activeKeyVelTab = 0;
};

// =============================================================================
// Keyboard Tab: Custom Keyboard, Oscilloscope
// =============================================================================
class KeyboardTabContent final : public juce::Component
{
public:
    explicit KeyboardTabContent (PluginProcessor& p);
    void resized() override;
    void paint (juce::Graphics& g) override;

private:
    Oscilloscope oscilloscope;
    CustomKeyboard keyboard;
};

// =============================================================================
// Modulation Tab: Modulation Matrix
// =============================================================================
class ModulationTabContent final : public juce::Component
{
public:
    explicit ModulationTabContent (PluginProcessor& p);
    void resized() override;
    void paint (juce::Graphics& g) override;

private:
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
    VibratoComponent vibratoComponent;
    VolumeComponent volumeComponent;
    NoiseComponent noiseComponent;
    ChorusMixComponent chorusMixComponent;
    PortamentoComponent portamentoComponent;
};

// =============================================================================
// Experiment Tab: Mono/Legato toggles
// =============================================================================
class ExperimentTabContent final : public juce::Component
{
public:
    explicit ExperimentTabContent (PluginProcessor& p);
    void resized() override;
    void paint (juce::Graphics& g) override;

private:
    juce::ToggleButton monoToggle { "Mono" };
    juce::ToggleButton legatoToggle { "Legato" };
    juce::ToggleButton gateModeToggle { "Gate Mode" };

    juce::Slider pitchBendRangeSlider;
    juce::Label pitchBendRangeLabel { {}, "Pitch Bend Range" };

    juce::Slider manualBpmSlider;
    juce::Label manualBpmLabel { {}, "Manual BPM" };

    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<ButtonAttachment> monoAttachment;
    std::unique_ptr<ButtonAttachment> legatoAttachment;
    std::unique_ptr<ButtonAttachment> gateModeAttachment;
    std::unique_ptr<SliderAttachment> pitchBendRangeAttachment;
    std::unique_ptr<SliderAttachment> manualBpmAttachment;
};
