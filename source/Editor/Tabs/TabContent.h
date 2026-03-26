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
#include "../ReverbPreDelayComponent.h"
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
                                     private juce::AudioProcessorParameter::Listener,
                                     private juce::Timer
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
        auto numArea = getLocalBounds().toFloat().reduced (2.0f, 2.0f);
        g.setColour (SECONDARY_COLOR);
        g.fillRoundedRectangle (numArea, 4.0f);
        g.setColour (TEXT_COLOR);
        g.drawRoundedRectangle (numArea, 4.0f, 1.0f);

        if (hoverAlpha < 0.99f)
        {
            g.setOpacity (1.0f - hoverAlpha);
            g.setFont (11.0f);
            g.drawText ("Pitch Bend", numArea, juce::Justification::centred, true);
        }

        if (hoverAlpha > 0.01f)
        {
            g.setOpacity (hoverAlpha);
            g.setFont (12.0f);
            int val = static_cast<int> (param->convertFrom0to1 (param->getValue()));
            g.drawText (juce::String (val), numArea, juce::Justification::centred, true);
        }
    }

    void mouseEnter (const juce::MouseEvent&) override { hoverTarget = true; startTimerHz (60); }
    void mouseExit (const juce::MouseEvent&) override { hoverTarget = false; startTimerHz (60); }

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

    void timerCallback() override
    {
        const float target = hoverTarget ? 1.0f : 0.0f;
        hoverAlpha += 0.25f * (target - hoverAlpha);
        if (std::abs (hoverAlpha - target) < 0.01f)
        {
            hoverAlpha = target;
            stopTimer();
        }
        repaint();
    }

    juce::RangedAudioParameter* param = nullptr;
    bool isDragging = false;
    float dragStartY = 0.0f;
    int dragStartValue = 0;
    float hoverAlpha = 0.0f;
    bool hoverTarget = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchBendRangeControl)
};

// =============================================================================
// VolumeControl: drag-to-adjust volume inline display
// =============================================================================
class VolumeControl final : public juce::Component,
                             private juce::AudioProcessorParameter::Listener,
                             private juce::Timer
{
public:
    explicit VolumeControl (juce::RangedAudioParameter* param)
        : param (param)
    {
        jassert (param != nullptr);
        param->addListener (this);
    }

    ~VolumeControl() override
    {
        if (param != nullptr)
            param->removeListener (this);
    }

    void paint (juce::Graphics& g) override
    {
        auto numArea = getLocalBounds().toFloat().reduced (2.0f, 2.0f);
        g.setColour (SECONDARY_COLOR);
        g.fillRoundedRectangle (numArea, 4.0f);
        g.setColour (TEXT_COLOR);
        g.drawRoundedRectangle (numArea, 4.0f, 1.0f);

        if (hoverAlpha < 0.99f)
        {
            g.setOpacity (1.0f - hoverAlpha);
            g.setFont (11.0f);
            g.drawText ("Volume", numArea, juce::Justification::centred, true);
        }

        if (hoverAlpha > 0.01f)
        {
            g.setOpacity (hoverAlpha);
            g.setFont (12.0f);
            int val = juce::roundToInt (param->convertFrom0to1 (param->getValue()) * 100.0f);
            g.drawText (juce::String (val), numArea, juce::Justification::centred, true);
        }
    }

    void mouseEnter (const juce::MouseEvent&) override { hoverTarget = true; startTimerHz (60); }
    void mouseExit (const juce::MouseEvent&) override { hoverTarget = false; startTimerHz (60); }

    void mouseDown (const juce::MouseEvent& e) override
    {
        isDragging = true;
        dragStartY = static_cast<float> (e.y);
        dragStartValue = param->getValue();
        param->beginChangeGesture();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (!isDragging) return;
        float deltaY = dragStartY - static_cast<float> (e.y);
        float newValue = juce::jlimit (0.0f, 1.0f, dragStartValue + deltaY / 200.0f);
        param->setValueNotifyingHost (newValue);
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

    void timerCallback() override
    {
        const float target = hoverTarget ? 1.0f : 0.0f;
        hoverAlpha += 0.25f * (target - hoverAlpha);
        if (std::abs (hoverAlpha - target) < 0.01f)
        {
            hoverAlpha = target;
            stopTimer();
        }
        repaint();
    }

    juce::RangedAudioParameter* param = nullptr;
    bool isDragging = false;
    float dragStartY = 0.0f;
    float dragStartValue = 0.0f;
    float hoverAlpha = 0.0f;
    bool hoverTarget = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VolumeControl)
};

// =============================================================================
// VoiceCountControl: click-to-select voice count via popup menu
// =============================================================================
class VoiceCountControl final : public juce::Component,
                                 private juce::AudioProcessorParameter::Listener,
                                 private juce::Timer
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
        auto numArea = getLocalBounds().toFloat().reduced (2.0f, 2.0f);
        g.setColour (SECONDARY_COLOR);
        g.fillRoundedRectangle (numArea, 4.0f);
        g.setColour (TEXT_COLOR);
        g.drawRoundedRectangle (numArea, 4.0f, 1.0f);

        if (hoverAlpha < 0.99f)
        {
            g.setOpacity (1.0f - hoverAlpha);
            g.setFont (11.0f);
            g.drawText ("Voices", numArea, juce::Justification::centred, true);
        }

        if (hoverAlpha > 0.01f)
        {
            g.setOpacity (hoverAlpha);
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
    }

    void mouseEnter (const juce::MouseEvent&) override { hoverTarget = true; startTimerHz (60); }
    void mouseExit (const juce::MouseEvent&) override { hoverTarget = false; startTimerHz (60); }

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

    void timerCallback() override
    {
        const float target = hoverTarget ? 1.0f : 0.0f;
        hoverAlpha += 0.25f * (target - hoverAlpha);
        if (std::abs (hoverAlpha - target) < 0.01f)
        {
            hoverAlpha = target;
            stopTimer();
        }
        repaint();
    }

    juce::AudioParameterChoice* voiceCountParam = nullptr;
    juce::AudioParameterBool* monoParam = nullptr;
    juce::AudioParameterBool* legatoParam = nullptr;
    float hoverAlpha = 0.0f;
    bool hoverTarget = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoiceCountControl)
};

// =============================================================================
// PortamentoBottomControl: drag-to-adjust portamento inline display
// =============================================================================
class PortamentoBottomControl final : public juce::Component,
                                       private juce::AudioProcessorParameter::Listener,
                                       private juce::Timer
{
public:
    explicit PortamentoBottomControl (juce::RangedAudioParameter* param)
        : param (param)
    {
        jassert (param != nullptr);
        param->addListener (this);
    }

    ~PortamentoBottomControl() override
    {
        if (param != nullptr)
            param->removeListener (this);
    }

    void paint (juce::Graphics& g) override
    {
        auto numArea = getLocalBounds().toFloat().reduced (2.0f, 2.0f);
        g.setColour (SECONDARY_COLOR);
        g.fillRoundedRectangle (numArea, 4.0f);
        g.setColour (TEXT_COLOR);
        g.drawRoundedRectangle (numArea, 4.0f, 1.0f);

        if (hoverAlpha < 0.99f)
        {
            g.setOpacity (1.0f - hoverAlpha);
            g.setFont (11.0f);
            g.drawText ("Porta", numArea, juce::Justification::centred, true);
        }

        if (hoverAlpha > 0.01f)
        {
            g.setOpacity (hoverAlpha);
            g.setFont (12.0f);
            float ms = param->convertFrom0to1 (param->getValue());
            juce::String text = ms < 1.0f ? "0 ms" : juce::String (static_cast<int> (ms)) + " ms";
            g.drawText (text, numArea, juce::Justification::centred, true);
        }
    }

    void mouseEnter (const juce::MouseEvent&) override { hoverTarget = true; startTimerHz (60); }
    void mouseExit (const juce::MouseEvent&) override { hoverTarget = false; startTimerHz (60); }

    void mouseDown (const juce::MouseEvent& e) override
    {
        isDragging = true;
        dragStartY = static_cast<float> (e.y);
        dragStartValue = param->getValue();
        param->beginChangeGesture();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (!isDragging) return;
        float deltaY = dragStartY - static_cast<float> (e.y);
        float newValue = juce::jlimit (0.0f, 1.0f, dragStartValue + deltaY / 200.0f);
        param->setValueNotifyingHost (newValue);
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

    void timerCallback() override
    {
        const float target = hoverTarget ? 1.0f : 0.0f;
        hoverAlpha += 0.25f * (target - hoverAlpha);
        if (std::abs (hoverAlpha - target) < 0.01f)
        {
            hoverAlpha = target;
            stopTimer();
        }
        repaint();
    }

    juce::RangedAudioParameter* param = nullptr;
    bool isDragging = false;
    float dragStartY = 0.0f;
    float dragStartValue = 0.0f;
    float hoverAlpha = 0.0f;
    bool hoverTarget = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PortamentoBottomControl)
};

// =============================================================================
// ScrollIndicator: overlay scroll thumb that fades in/out
// =============================================================================
class ScrollIndicator final : public juce::Component,
                               private juce::Timer
{
public:
    ScrollIndicator() { setInterceptsMouseClicks (false, false); }

    void setViewport (juce::Viewport* vp) { viewport = vp; }

    void showIndicator()
    {
        alpha = 1.0f;
        fadeDelayActive = true;
        startTimer (1000);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        if (viewport == nullptr || alpha <= 0.0f) return;

        auto viewHeight = (float) viewport->getHeight();
        auto contentHeight = (float) viewport->getViewedComponent()->getHeight();
        if (contentHeight <= viewHeight) return;

        float ratio = viewHeight / contentHeight;
        float thumbHeight = juce::jmax (20.0f, ratio * viewHeight);
        float scrollRatio = (float) viewport->getViewPositionY() / (contentHeight - viewHeight);
        float thumbY = scrollRatio * (viewHeight - thumbHeight);
        constexpr float barWidth = 4.0f;
        float barX = (float) getWidth() - barWidth - 2.0f;

        g.setColour (TEXT_COLOR.withAlpha (alpha * 0.5f));
        g.fillRoundedRectangle (barX, thumbY, barWidth, thumbHeight, barWidth / 2.0f);
    }

private:
    void timerCallback() override
    {
        if (fadeDelayActive)
        {
            fadeDelayActive = false;
            startTimer (30);
            return;
        }
        alpha -= 0.1f;
        if (alpha <= 0.0f)
        {
            alpha = 0.0f;
            stopTimer();
        }
        repaint();
    }

    juce::Viewport* viewport = nullptr;
    float alpha = 0.0f;
    bool fadeDelayActive = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScrollIndicator)
};

// =============================================================================
// NotifyingViewport: Viewport that notifies on scroll
// =============================================================================
class NotifyingViewport final : public juce::Viewport
{
public:
    std::function<void()> onScroll;

    void visibleAreaChanged (const juce::Rectangle<int>& newVisibleArea) override
    {
        juce::Viewport::visibleAreaChanged (newVisibleArea);
        if (onScroll) onScroll();
    }
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
    NotifyingViewport viewport;
    juce::Component scrollContent;
    ScrollIndicator scrollIndicator;

    // Synth components
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

    // Effects components
    DelayComponent delayComponent;
    DelayModComponent delayModComponent;
    DelayRateComponent delayRateComponent;
    CustomToggleComponent delayBpmToggle;
    DelayHighpassComponent delayHighpassComponent;
    DelayLowpassComponent delayLowpassComponent;

    ReverbComponent reverbComponent;
    ReverbModComponent reverbModComponent;
    ReverbSizeComponent reverbSizeComponent;
    ReverbPreDelayComponent reverbPreDelayComponent;
    ReverbBassMultComponent reverbBassMultComponent;
    ReverbDampingComponent reverbDampingComponent;
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
    NotifyingViewport matrixViewport;
    ScrollIndicator scrollIndicator;
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
    juce::Slider reverbModDepthSlider, reverbDiffusionSlider, reverbPreDelaySlider, reverbWidthSlider, reverbLevelSlider;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> manualBpmAttachment;

    // Reverb attachments
    std::unique_ptr<SliderAttachment> reverbDecayAttachment, reverbSizeAttachment, reverbDampingAttachment;
    std::unique_ptr<SliderAttachment> reverbBassMultAttachment, reverbModRateAttachment, reverbModDepthAttachment;
    std::unique_ptr<SliderAttachment> reverbDiffusionAttachment, reverbPreDelayAttachment;
    std::unique_ptr<SliderAttachment> reverbWidthAttachment, reverbLevelAttachment;

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

#if SYNTHDEMO_DEV_MODE
    PluginProcessor& processorRef;
    juce::Label presetSectionLabel { {}, "Preset Manager (Dev)" };
    juce::TextEditor presetNameEditor;
    juce::ComboBox categoryCombo;
    juce::TextButton savePresetButton { "Save Preset" };
#endif
};
