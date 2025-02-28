#include "ParameterDial.h"

ParameterDial::ParameterDial (PluginProcessor& p, juce::StringRef id, juce::StringRef label) : processor (p)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 100, 25);
    addAndMakeVisible (slider);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.parameters, id, slider);
    addAndMakeVisible (parameterLabel);
    parameterLabel.setText (label, juce::dontSendNotification);
    parameterLabel.setJustificationType (juce::Justification::centred);

    setSize (200, 300);
}

void ParameterDial::paint (juce::Graphics& g)
{
    // g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    //
    // g.setColour (juce::Colours::white);
    // g.setFont (16.0f);
}

void ParameterDial::resized()
{
    auto container = getLocalBounds();
    parameterLabel.setBounds (container.removeFromBottom (20));
    slider.setBounds (container.reduced (5));
}
