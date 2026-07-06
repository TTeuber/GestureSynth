#include "ParameterDial.h"

ParameterDial::ParameterDial (PluginProcessor& p, juce::StringRef id, const juce::StringRef label) : processor (p), labelText (label)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 100, 25);
    addAndMakeVisible (slider);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.parameters, id, slider);
    addAndMakeVisible (parameterLabel);
    parameterLabel.setText (label, juce::dontSendNotification);
    parameterLabel.setJustificationType (juce::Justification::centred);

    slider.onValueChange = [this] {
        parameterLabel.setText (juce::String (slider.getValue()), juce::dontSendNotification);
    };

    slider.onDragEnd = [this] {
        parameterLabel.setText (labelText, juce::dontSendNotification);
    };

    parameterLabel.setEditable (true);
    parameterLabel.onEditorHide = [this] {
        slider.setValue (parameterLabel.getText().getFloatValue());
    };

    setSize (200, 300);
}

void ParameterDial::paint (juce::Graphics& /*g*/)
{
}

void ParameterDial::resized()
{
    auto container = getLocalBounds();
    parameterLabel.setBounds (container.removeFromBottom (20));
    slider.setBounds (container.reduced (5));
}
