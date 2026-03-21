#pragma once

#include "../../PluginProcessor.h"
#include "../../Theme.h"
#include "../ADSRGraph.h"
#include "../ChorusComponent.h"
#include "../DelayComponent.h"
#include "../DelayFilterComponent.h"
#include "../DelayModComponent.h"
#include "../DelayRateComponent.h"
#include "../ReverbComponent.h"
#include "../ReverbFilterComponent.h"
#include "../ReverbModComponent.h"
#include "../ReverbSizeComponent.h"
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
#include "../Oscilloscope.h"
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
        g.setColour (SECONDARY_COLOR);
        g.fillRoundedRectangle (bounds, 3.0f);

        if (selected)
        {
            g.setColour (getModColor (sourceID));
            g.drawRoundedRectangle (bounds.reduced (0.5f), 3.0f, 1.5f);
        }

        bool isTarget = modModeState != nullptr
            && modModeState->isModulationMode()
            && modModeState->getTargetSourceID() == sourceID;

        if (compactMode && !hovered)
        {
            // Compact default: text only, centered
            g.setColour (isTarget ? getModColor (sourceID) : TEXT_COLOR);
            g.setFont (13.0f);
            g.drawText (text, bounds, juce::Justification::centred, true);
        }
        else if (compactMode && hovered)
        {
            // Compact hovered: crosshair only, centered
            float cx = bounds.getCentreX();
            float cy = bounds.getCentreY();
            float r = 16.0f * 0.35f;

            g.setColour (isTarget ? getModColor (sourceID) : TEXT_COLOR.withAlpha (0.6f));
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

            g.setColour (isTarget ? getModColor (sourceID) : TEXT_COLOR.withAlpha (0.6f));
            g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 1.5f);
            g.drawLine (cx - r - 2, cy, cx + r + 2, cy, 1.2f);
            g.drawLine (cx, cy - r - 2, cx, cy + r + 2, 1.2f);

            g.setColour (TEXT_COLOR);
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
    VoiceCountControl (juce::AudioParameterChoice* voiceCountParam,
                       juce::AudioParameterBool* monoParam,
                       juce::AudioParameterBool* legatoParam)
        : voiceCountParam (voiceCountParam), monoParam (monoParam), legatoParam (legatoParam)
    {
        jassert (voiceCountParam != nullptr);
        jassert (monoParam != nullptr);
        jassert (legatoParam != nullptr);
        voiceCountParam->addListener (this);
        monoParam->addListener (this);
        legatoParam->addListener (this);
    }

    ~VoiceCountControl() override
    {
        if (voiceCountParam != nullptr) voiceCountParam->removeListener (this);
        if (monoParam != nullptr) monoParam->removeListener (this);
        if (legatoParam != nullptr) legatoParam->removeListener (this);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Label on left
        auto labelArea = bounds.removeFromLeft (bounds.getWidth() * 0.5f);
        g.setColour (TEXT_COLOR);
        g.setFont (11.0f);
        g.drawText ("Voices", labelArea, juce::Justification::centredRight, true);

        // Display text in rounded rect on right
        auto numArea = bounds.reduced (2.0f, 2.0f);
        g.setColour (SECONDARY_COLOR);
        g.fillRoundedRectangle (numArea, 4.0f);
        g.setColour (TEXT_COLOR);
        g.drawRoundedRectangle (numArea, 4.0f, 1.0f);
        g.setFont (12.0f);

        juce::String displayText;
        if (legatoParam->get())
            displayText = "Legato";
        else if (monoParam->get())
            displayText = "Mono";
        else
            displayText = voiceCountParam->getCurrentValueAsText();

        g.drawText (displayText, numArea, juce::Justification::centred, true);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (!getLocalBounds().contains (e.x, e.y))
            return;

        bool isMono = monoParam->get();
        bool isLegato = legatoParam->get();

        juce::PopupMenu menu;
        menu.addItem (2, "Mono", true, isMono && !isLegato);
        menu.addItem (1, "Legato", true, isLegato);
        menu.addSeparator();

        const auto& choices = voiceCountParam->choices;
        for (int i = 0; i < choices.size(); ++i)
            menu.addItem (i + 100, choices[i], true, !isMono && !isLegato && i == voiceCountParam->getIndex());

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
            [this] (int result)
            {
                if (result == 1) // Legato
                {
                    monoParam->beginChangeGesture();
                    *monoParam = true;
                    monoParam->endChangeGesture();
                    legatoParam->beginChangeGesture();
                    *legatoParam = true;
                    legatoParam->endChangeGesture();
                }
                else if (result == 2) // Mono
                {
                    monoParam->beginChangeGesture();
                    *monoParam = true;
                    monoParam->endChangeGesture();
                    legatoParam->beginChangeGesture();
                    *legatoParam = false;
                    legatoParam->endChangeGesture();
                }
                else if (result >= 100) // Voice count
                {
                    monoParam->beginChangeGesture();
                    *monoParam = false;
                    monoParam->endChangeGesture();
                    legatoParam->beginChangeGesture();
                    *legatoParam = false;
                    legatoParam->endChangeGesture();
                    voiceCountParam->beginChangeGesture();
                    voiceCountParam->setValueNotifyingHost (voiceCountParam->convertTo0to1 (static_cast<float> (result - 100)));
                    voiceCountParam->endChangeGesture();
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

    juce::AudioParameterChoice* voiceCountParam = nullptr;
    juce::AudioParameterBool* monoParam = nullptr;
    juce::AudioParameterBool* legatoParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoiceCountControl)
};

// =============================================================================
// Main Tab: Filter, Chorus, Vibrato, Waveform, Sub Osc, Detune, HPF
// =============================================================================
class MainTabContent final : public juce::Component
{
public:
    explicit MainTabContent (PluginProcessor& p, ModulationModeState* modState = nullptr, AnimationFrameSource* animSource = nullptr);
    void resized() override;
    void paint (juce::Graphics& g) override;

private:
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
};

// =============================================================================
// Keyboard Tab: Oscilloscope only (keyboard is persistent)
// =============================================================================
class KeyboardTabContent final : public juce::Component
{
public:
    explicit KeyboardTabContent (PluginProcessor& p, AnimationFrameSource* animSource = nullptr);
    void resized() override;
    void paint (juce::Graphics& g) override;

private:
    Oscilloscope oscilloscope;
};

// =============================================================================
// Modulation Tab: Modulation Matrix with scrollable viewport
// =============================================================================
class ModulationTabContent final : public juce::Component
{
public:
    explicit ModulationTabContent (PluginProcessor& p, AnimationFrameSource* animSource = nullptr);
    void resized() override;
    void paint (juce::Graphics& g) override;

private:
    MatrixComponent matrixComponent;
    juce::Viewport matrixViewport;
};

// =============================================================================
// Effects Tab: Chorus, Vibrato, Volume, Noise, ChorusMix, Portamento
// =============================================================================
class EffectsTabContent final : public juce::Component
{
public:
    explicit EffectsTabContent (PluginProcessor& p, ModulationModeState* modState = nullptr, AnimationFrameSource* animSource = nullptr);
    void resized() override;
    void paint (juce::Graphics& g) override;

private:
    DelayComponent delayComponent;
    DelayModComponent delayModComponent;
    DelayRateComponent delayRateComponent;
    CustomToggleComponent delayBpmToggle;
    DelayHighpassComponent delayHighpassComponent;
    DelayLowpassComponent delayLowpassComponent;

    ReverbComponent reverbComponent;
    ReverbModComponent reverbModComponent;
    ReverbSizeComponent reverbSizeComponent;
    SingleParameterComponent reverbPreDelayComponent;
    ReverbBassMultComponent reverbBassMultComponent;
    ReverbDampingComponent reverbDampingComponent;
};

// =============================================================================
// Experiment Tab: Reverb + BBD Delay side-by-side
// =============================================================================
class ExperimentTabContent final : public juce::Component
{
public:
    explicit ExperimentTabContent (PluginProcessor& p);
    void resized() override;
    void paint (juce::Graphics& g) override;

private:
    juce::Slider manualBpmSlider;
    juce::Label manualBpmLabel { {}, "Manual BPM" };

    // Reverb sliders
    juce::Slider reverbDecaySlider, reverbSizeSlider, reverbDampingSlider, reverbBassMultSlider, reverbModRateSlider;
    juce::Slider reverbModDepthSlider, reverbDiffusionSlider, reverbPreDelaySlider, reverbWidthSlider, reverbMixSlider;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> manualBpmAttachment;

    // Reverb attachments
    std::unique_ptr<SliderAttachment> reverbDecayAttachment, reverbSizeAttachment, reverbDampingAttachment;
    std::unique_ptr<SliderAttachment> reverbBassMultAttachment, reverbModRateAttachment, reverbModDepthAttachment;
    std::unique_ptr<SliderAttachment> reverbDiffusionAttachment, reverbPreDelayAttachment;
    std::unique_ptr<SliderAttachment> reverbWidthAttachment, reverbMixAttachment;

    // Delay sliders
    juce::Slider delayTimeSlider, delayFeedbackSlider, delayMixSlider, delayLowpassSlider, delayHighpassSlider;
    juce::Slider delaySaturationSlider, delayModRateSlider, delayModDepthSlider, delayDiffusionSlider;

    // Delay attachments
    std::unique_ptr<SliderAttachment> delayTimeAttachment, delayFeedbackAttachment, delayMixAttachment;
    std::unique_ptr<SliderAttachment> delayLowpassAttachment, delayHighpassAttachment, delaySaturationAttachment;
    std::unique_ptr<SliderAttachment> delayModRateAttachment, delayModDepthAttachment, delayDiffusionAttachment;

    // Delay combo box (note division)
    juce::ComboBox delayNoteDivisionCombo;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ComboBoxAttachment> delayNoteDivisionAttachment;

    // Delay toggles
    std::unique_ptr<CustomToggleComponent> delayTempoSyncToggle;
    std::unique_ptr<CustomToggleComponent> delayPingPongToggle;

    // On/off toggles
    std::unique_ptr<CustomToggleComponent> delayOnToggle;
    std::unique_ptr<CustomToggleComponent> reverbOnToggle;
};
