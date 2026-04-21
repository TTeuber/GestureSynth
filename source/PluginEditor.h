#pragma once

#include "Editor/Tabs/TabContent.h"
#include "Editor/Utility/AnimationFrameSource.h"
#include "Editor/Utility/CustomLookAndFeel.h"
#include "Editor/Utility/ModulationModeState.h"
#include "Editor/Utility/PaintHelpers.h"
#include "PluginProcessor.h"
#include "Theme.h"
#include <juce_audio_utils/juce_audio_utils.h>

#define HEIGHT 800
#define WIDTH 1100

//==============================================================================
class BoxBackground : public juce::Component
{
public:
    void paint (juce::Graphics& g) override
    {
        g.fillAll (PRIMARY_COLOR);
        PaintHelpers::drawComponentBox (g, getLocalBounds().toFloat());
    }
};

//==============================================================================
class ResizeGrip : public juce::ResizableCornerComponent
{
public:
    using juce::ResizableCornerComponent::ResizableCornerComponent;

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour (TEXT_COLOR.withAlpha (0.3f));

        for (int i = 0; i < 3; ++i)
        {
            float offset = 4.0f * (float) (i + 1);
            g.drawLine (bounds.getRight() - offset, bounds.getBottom(),
                        bounds.getRight(), bounds.getBottom() - offset, 1.0f);
        }
    }
};

//==============================================================================
class RedBorderButton final : public juce::TextButton
{
public:
    using juce::TextButton::TextButton;

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        juce::TextButton::paintButton (g, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
        g.setColour (juce::Colour (0xffd03030));
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.75f), 6.0f, 1.5f);
    }
};

//==============================================================================
class HamburgerButton final : public juce::TextButton
{
public:
    using juce::TextButton::TextButton;

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool /*shouldDrawButtonAsDown*/) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (0.5f);
        auto bg = TERTIARY_COLOR;
        if (shouldDrawButtonAsHighlighted)
            bg = bg.brighter (0.08f);
        g.setColour (bg);
        g.fillRoundedRectangle (bounds, Style::radiusSmall);

        auto inner = bounds.reduced (bounds.getWidth() * 0.28f, bounds.getHeight() * 0.30f);
        g.setColour (TEXT_COLOR);
        float cx1 = inner.getX();
        float cx2 = inner.getRight();
        float thickness = juce::jmax (1.0f, inner.getHeight() * 0.12f);
        for (int i = 0; i < 3; ++i)
        {
            float t = (float) i / 2.0f;
            float y = inner.getY() + t * inner.getHeight();
            g.drawLine (cx1, y, cx2, y, thickness);
        }
    }
};

//==============================================================================
class PluginEditor final : public juce::AudioProcessorEditor,
                           public juce::MidiKeyboardStateListener,
                           public ModulationModeState::Listener,
                           private juce::Timer
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    static constexpr float kMinScale = 0.5f;
    static constexpr float kMaxScale = 1.5f;

    float scaleFactor = 1.0f;

    PluginProcessor& processorRef;

    ModulationModeState modModeState;
    AnimationFrameSource animationSource;

    juce::Component contentWrapper;

    std::unique_ptr<MainTabContent> mainTab;
    std::unique_ptr<KeyboardTabContent> keyboardTab;
    std::unique_ptr<ModulationTabContent> modulationTab;
    std::unique_ptr<ExperimentTabContent> experimentTab;

    CustomLookAndFeel customLookAndFeel;
    juce::TabbedComponent tabbedComponent { juce::TabbedButtonBar::TabsAtTop };

    // Persistent bottom panel and its components
    juce::Component persistentPanel;

    // Keyboard row
    BoxBackground wheelBoxBackground;
    ModWheelComponent modWheel;
    PitchWheelComponent pitchWheel;
    CustomKeyboard keyboard;
    KeyVelComponent keyVelComponent;

    // Bottom control row
    PitchBendRangeControl pitchBendRangeControl;
    VoiceCountControl voiceCountControl;
    VolumeControl volumeControl;
    PortamentoBottomControl portamentoBottomControl;
    juce::TextButton prevPresetButton { "<" };
    juce::TextButton presetButton { "Presets" };
    juce::TextButton nextPresetButton { ">" };
    HamburgerButton menuButton;
    RedBorderButton panicButton { "Panic" };

    void loadPresetByFile (const juce::File& file);
    void navigatePreset (int direction);

    // LFO/ENV row
    LFOComponent lfoComponent;
    ADSRGraph adsrGraph;

    ModSourceTab mwTab, atTab, expTab;
    ModSourceTab lfoTabs[4];
    ModSourceTab envTabs[4];
    ModSourceTab velTab, keyTab;
    int activeLfoIndex = 0;
    int activeEnvIndex = 0;
    int activeKeyVelTab = 0;

    void selectLfo (int index);
    void selectEnv (int index);
    void selectKeyVel (int index);

    juce::ComponentBoundsConstrainer constrainer;
    std::unique_ptr<ResizeGrip> resizeGrip;

    bool keyPressed (const juce::KeyPress& key) override;
    void timerCallback() override;

    void handleNoteOn (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float /*velocity*/) override;

    // ModulationModeState::Listener
    void modulationModeChanged (ModulationModeState::Mode newMode) override;
    void targetSourceChanged (const juce::String& newSourceID) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
