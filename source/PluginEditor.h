#pragma once

#include "Editor/Tabs/TabContent.h"
#include "Editor/Utility/ModulationModeState.h"
#include "PluginProcessor.h"
#include "Theme.h"
#include <juce_audio_utils/juce_audio_utils.h>

#define HEIGHT 840
#define WIDTH 1100

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
    int windowHeight = HEIGHT;
    int windowWidth = WIDTH;

    PluginProcessor& processorRef;

    ModulationModeState modModeState;
    juce::Label modeLabel;

    std::unique_ptr<MainTabContent> mainTab;
    std::unique_ptr<KeyboardTabContent> keyboardTab;
    std::unique_ptr<ModulationTabContent> modulationTab;
    std::unique_ptr<EffectsTabContent> effectsTab;
    std::unique_ptr<ExperimentTabContent> experimentTab;

    juce::TabbedComponent tabbedComponent { juce::TabbedButtonBar::TabsAtTop };

    bool keyPressed (const juce::KeyPress& key) override;
    void timerCallback() override;

    void handleNoteOn (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float /*velocity*/) override;

    // ModulationModeState::Listener
    void modulationModeChanged (ModulationModeState::Mode newMode) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
