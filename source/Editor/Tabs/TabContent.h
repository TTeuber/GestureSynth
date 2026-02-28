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
#include "../Utility/CustomToggleComponent.h"
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

    void setCompactMode (bool compact) { compactMode = compact; }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Background
        if (selected)
            g.setColour (juce::Colours::orange);
        else
            g.setColour (SECONDARY_COLOR);
        g.fillRoundedRectangle (bounds, 3.0f);

        bool isTarget = modModeState != nullptr
            && modModeState->isModulationMode()
            && modModeState->getTargetSourceID() == sourceID;

        if (compactMode && !hovered)
        {
            // Compact default: text only, centered
            g.setColour (isTarget ? MOD_COLOR : (selected ? juce::Colours::black : TEXT_COLOR));
            g.setFont (13.0f);
            g.drawText (text, bounds, juce::Justification::centred, true);
        }
        else if (compactMode && hovered)
        {
            // Compact hovered: crosshair only, centered
            float cx = bounds.getCentreX();
            float cy = bounds.getCentreY();
            float r = 16.0f * 0.35f;

            g.setColour (isTarget ? MOD_COLOR : TEXT_COLOR.withAlpha (0.6f));
            g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 1.5f);
            g.drawLine (cx - r - 2, cy, cx + r + 2, cy, 1.2f);
            g.drawLine (cx, cy - r - 2, cx, cy + r + 2, 1.2f);
        }
        else
        {
            // Normal: crosshair left, text right
            const float iconSize = 16.0f;
            auto iconArea = bounds.withWidth (iconSize + 8).reduced (4.0f);
            float cx = iconArea.getCentreX();
            float cy = iconArea.getCentreY();
            float r = iconSize * 0.35f;

            g.setColour (isTarget ? MOD_COLOR : TEXT_COLOR.withAlpha (0.6f));
            g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 1.5f);
            g.drawLine (cx - r - 2, cy, cx + r + 2, cy, 1.2f);
            g.drawLine (cx, cy - r - 2, cx, cy + r + 2, 1.2f);

            g.setColour (selected ? juce::Colours::black : TEXT_COLOR);
            g.setFont (13.0f);
            g.drawText (text, bounds.withTrimmedLeft (iconSize + 8), juce::Justification::centred, true);
        }
    }

    void mouseEnter (const juce::MouseEvent&) override
    {
        if (compactMode) { hovered = true; repaint(); }
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        if (compactMode) { hovered = false; repaint(); }
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (!getLocalBounds().contains (e.x, e.y))
            return;

        // In compact mode, the whole tab acts as crosshair when hovered
        bool clickedCrosshair = compactMode ? hovered : (e.x < 24);

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
                if (clickedCrosshair && modModeState->getTargetSourceID() == sourceID)
                {
                    modModeState->setMode (ModulationModeState::Mode::Normal);
                    return;
                }
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
    bool compactMode = false;
    bool hovered = false;
};

// =============================================================================
// PitchBendRangeControl: drag-to-adjust pitch bend range inline display
// =============================================================================
class PitchBendRangeControl final : public juce::Component,
                                     private juce::AudioProcessorParameter::Listener
{
public:
    explicit PitchBendRangeControl (juce::RangedAudioParameter* param)
        : param (param)
    {
        jassert (param != nullptr);
        param->addListener (this);
    }

    ~PitchBendRangeControl() override
    {
        if (param != nullptr)
            param->removeListener (this);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Label on left
        auto labelArea = bounds.removeFromLeft (bounds.getWidth() * 0.6f);
        g.setColour (TEXT_COLOR);
        g.setFont (11.0f);
        g.drawText ("Pitch Bend", labelArea, juce::Justification::centredRight, true);

        // Number in rounded rect on right
        auto numArea = bounds.reduced (2.0f, 2.0f);
        g.setColour (SECONDARY_COLOR);
        g.fillRoundedRectangle (numArea, 4.0f);
        g.setColour (TEXT_COLOR);
        g.drawRoundedRectangle (numArea, 4.0f, 1.0f);
        g.setFont (12.0f);
        int val = static_cast<int> (param->convertFrom0to1 (param->getValue()));
        g.drawText (juce::String (val), numArea, juce::Justification::centred, true);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        isDragging = true;
        dragStartY = static_cast<float> (e.y);
        dragStartValue = static_cast<int> (param->convertFrom0to1 (param->getValue()));
        param->beginChangeGesture();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (!isDragging) return;
        float deltaY = dragStartY - static_cast<float> (e.y);
        int steps = static_cast<int> (deltaY / 20.0f);
        int newRange = juce::jlimit (1, 12, dragStartValue + steps);
        float normalised = param->convertTo0to1 (static_cast<float> (newRange));
        param->setValueNotifyingHost (normalised);
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (isDragging)
        {
            param->endChangeGesture();
            isDragging = false;
        }
    }

    juce::MouseCursor getMouseCursor() override
    {
        return juce::MouseCursor::UpDownResizeCursor;
    }

private:
    void parameterValueChanged (int, float) override
    {
        juce::MessageManager::callAsync ([this] { repaint(); });
    }
    void parameterGestureChanged (int, bool) override {}

    juce::RangedAudioParameter* param = nullptr;
    bool isDragging = false;
    float dragStartY = 0.0f;
    int dragStartValue = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchBendRangeControl)
};

// =============================================================================
// VoiceCountControl: click-to-select voice count via popup menu
// =============================================================================
class VoiceCountControl final : public juce::Component,
                                 private juce::AudioProcessorParameter::Listener
{
public:
    explicit VoiceCountControl (juce::AudioParameterChoice* param)
        : param (param)
    {
        jassert (param != nullptr);
        param->addListener (this);
    }

    ~VoiceCountControl() override
    {
        if (param != nullptr)
            param->removeListener (this);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Label on left
        auto labelArea = bounds.removeFromLeft (bounds.getWidth() * 0.5f);
        g.setColour (TEXT_COLOR);
        g.setFont (11.0f);
        g.drawText ("Voices", labelArea, juce::Justification::centredRight, true);

        // Number in rounded rect on right
        auto numArea = bounds.reduced (2.0f, 2.0f);
        g.setColour (SECONDARY_COLOR);
        g.fillRoundedRectangle (numArea, 4.0f);
        g.setColour (TEXT_COLOR);
        g.drawRoundedRectangle (numArea, 4.0f, 1.0f);
        g.setFont (12.0f);
        g.drawText (param->getCurrentValueAsText(), numArea, juce::Justification::centred, true);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (!getLocalBounds().contains (e.x, e.y))
            return;

        juce::PopupMenu menu;
        const auto& choices = param->choices;
        for (int i = 0; i < choices.size(); ++i)
            menu.addItem (i + 1, choices[i], true, i == param->getIndex());

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
            [this] (int result)
            {
                if (result > 0)
                {
                    param->beginChangeGesture();
                    param->setValueNotifyingHost (param->convertTo0to1 (static_cast<float> (result - 1)));
                    param->endChangeGesture();
                }
            });
    }

    juce::MouseCursor getMouseCursor() override
    {
        return juce::MouseCursor::PointingHandCursor;
    }

private:
    void parameterValueChanged (int, float) override
    {
        juce::MessageManager::callAsync ([this] { repaint(); });
    }
    void parameterGestureChanged (int, bool) override {}

    juce::AudioParameterChoice* param = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoiceCountControl)
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

    // Bottom control row
    PitchBendRangeControl pitchBendRangeControl;
    VoiceCountControl voiceCountControl;
    CustomToggleComponent monoToggle;
    CustomToggleComponent legatoToggle;
    CustomToggleComponent gateToggle;
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
