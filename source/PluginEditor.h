#pragma once

#include "Editor/Tabs/TabContent.h"
#include "PluginProcessor.h"
#include "Theme.h"
#include <juce_audio_utils/juce_audio_utils.h>

#define HEIGHT 840
#define WIDTH 1100

//==============================================================================
class PluginEditor final : public juce::AudioProcessorEditor, public juce::MidiKeyboardStateListener, private juce::Timer
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

    juce::MidiKeyboardComponent keyboardComponent;

    std::unique_ptr<MainTabContent> mainTab;
    std::unique_ptr<OscillatorTabContent> oscillatorTab;
    std::unique_ptr<ModulationTabContent> modulationTab;
    std::unique_ptr<EffectsTabContent> effectsTab;
    std::unique_ptr<ExperimentTabContent> experimentTab;

    juce::TabbedComponent tabbedComponent { juce::TabbedButtonBar::TabsAtTop };

    bool keyPressed (const juce::KeyPress& key) override;
    void timerCallback() override;

    void handleNoteOn (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float /*velocity*/) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
