#pragma once

#include "../PluginProcessor.h"
#include <juce_audio_utils/juce_audio_utils.h>

class PluginEditor;

class ParameterDial final : public juce::Component
{
public:
    ParameterDial (PluginProcessor& p, juce::StringRef id, juce::StringRef label);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    PluginProcessor& processor;

    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    juce::StringRef labelText;
    juce::Label parameterLabel;
};
