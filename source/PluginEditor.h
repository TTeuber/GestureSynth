#pragma once

#include "BinaryData.h"
#include "PluginProcessor.h"
#include "melatonin_inspector/melatonin_inspector.h"
#include <juce_audio_utils/juce_audio_utils.h>

//==============================================================================
class PluginEditor : public juce::AudioProcessorEditor, public juce::MidiKeyboardStateListener
{
public:
    explicit PluginEditor (PluginProcessor&);
    struct Dial
    {
        juce::Slider& slider;
        juce::String paramId;
        juce::Label& label;
        juce::String labelText;
        PluginProcessor& processorRef;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> addDial (Dial&);
    ~PluginEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    PluginProcessor& processorRef;
    // std::unique_ptr<melatonin::Inspector> inspector;
    // juce::TextButton inspectButton { "Inspect the UI" };

    juce::MidiKeyboardComponent keyboardComponent;

    juce::Slider volumeSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAttachment;
    juce::Label volumeLabel;

    juce::Slider ampAttackSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ampAttackAttachment;
    juce::Label ampAttackLabel;

    juce::Slider ampDecaySlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ampDecayAttachment;
    juce::Label ampDecayLabel;

    juce::Slider ampSustainSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ampSustainAttachment;
    juce::Label ampSustainLabel;

    juce::Slider ampReleaseSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ampReleaseAttachment;
    juce::Label ampReleaseLabel;

    juce::Slider filterFrequencySlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterFrequencyAttachment;
    juce::Label filterFrequencyLabel;

    juce::Slider filterEnvelopeAmountSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterEnvelopeAmountAttachment;
    juce::Label filterEnvelopeAmountLabel;

    juce::Slider filterResonanceSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterResonanceAttachment;
    juce::Label filterResonanceLabel;

    void handleNoteOn (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float /*velocity*/) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
