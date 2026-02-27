#pragma once

#include "Editor/Tabs/TabContent.h"
#include "Editor/Utility/ModulationModeState.h"
#include "PluginProcessor.h"
#include "Theme.h"
#include <juce_audio_utils/juce_audio_utils.h>

#define HEIGHT 840
#define WIDTH 1000

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

    juce::Component contentWrapper;
    juce::Label modeLabel;
    juce::Label scaleLabel;

    std::unique_ptr<MainTabContent> mainTab;
    std::unique_ptr<KeyboardTabContent> keyboardTab;
    std::unique_ptr<ModulationTabContent> modulationTab;
    std::unique_ptr<EffectsTabContent> effectsTab;
    std::unique_ptr<ExperimentTabContent> experimentTab;

    juce::TabbedComponent tabbedComponent { juce::TabbedButtonBar::TabsAtTop };

    juce::ComponentBoundsConstrainer constrainer;
    std::unique_ptr<ResizeGrip> resizeGrip;

    bool keyPressed (const juce::KeyPress& key) override;
    void timerCallback() override;

    void handleNoteOn (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float /*velocity*/) override;

    // ModulationModeState::Listener
    void modulationModeChanged (ModulationModeState::Mode newMode) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
