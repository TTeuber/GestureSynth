#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), keyboardComponent (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    processorRef.keyboardState.addListener (this);
    addAndMakeVisible (keyboardComponent);

    Dial volumeDial { volumeSlider, "volume", volumeLabel, "Volume", processorRef, (std::move (volumeAttachment)) };
    addDial (volumeDial);

    Dial ampAttackDial { ampAttackSlider, "ampAttack", ampAttackLabel, "Amp Attack", processorRef, (std::move (ampAttackAttachment)) };
    addDial (ampAttackDial);

    Dial ampDecayDial { ampDecaySlider, "ampDecay", ampDecayLabel, "Amp Decay", processorRef, (std::move (ampDecayAttachment)) };
    addDial (ampDecayDial);

    Dial ampSustainDial { ampSustainSlider, "ampSustain", ampSustainLabel, "Amp Sustain", processorRef, (std::move (ampSustainAttachment)) };
    addDial (ampSustainDial);

    Dial ampReleaseDial { ampReleaseSlider, "ampRelease", ampReleaseLabel, "Amp Release", processorRef, (std::move (ampReleaseAttachment)) };
    addDial (ampReleaseDial);

    Dial filterFrequencyDial { filterFrequencySlider, "filterFrequency", filterFrequencyLabel, "Filter Frequency", processorRef, (std::move (filterFrequencyAttachment)) };
    addDial (filterFrequencyDial);

    Dial filterEnvelopeAmountDial { filterEnvelopeAmountSlider, "filterEnvelopeAmount", filterEnvelopeAmountLabel, "Filter Envelope Amount", processorRef, (std::move (filterEnvelopeAmountAttachment)) };
    addDial (filterEnvelopeAmountDial);

    Dial filterResonanceDial { filterResonanceSlider, "filterResonance", filterResonanceLabel, "Filter Resonance", processorRef, (std::move (filterResonanceAttachment)) };
    addDial (filterResonanceDial);

    setSize (800, 600);
}

std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> PluginEditor::addDial (Dial& d)
{
    d.slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    d.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 100, 25);
    addAndMakeVisible (d.slider);
    d.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.parameters, d.paramId, d.slider);
    addAndMakeVisible (d.label);
    d.label.setText (d.labelText, juce::dontSendNotification);
    d.label.setJustificationType (juce::Justification::centred);

    return std::move (d.attachment);
}

PluginEditor::~PluginEditor()
{
    processorRef.keyboardState.removeListener (this);
}

void PluginEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (16.0f);
}

void PluginEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromBottom (50);
    keyboardComponent.setBounds (10, getHeight() - 80, getWidth() - 20, 70);

    int containerHeight = 120;
    auto containerA = area.removeFromTop (containerHeight);
    auto containerB = area.removeFromTop (containerHeight);

    auto placeDial = [] (juce::Label& label, juce::Slider& slider, juce::Rectangle<int>& container) {
        auto subContainer = container.removeFromLeft (container.getWidth() / 4);
        label.setBounds (subContainer.removeFromBottom (20));
        slider.setBounds (subContainer.reduced (5));
    };

    placeDial (volumeLabel, volumeSlider, containerA);
    placeDial (filterFrequencyLabel, filterFrequencySlider, containerA);
    placeDial (filterEnvelopeAmountLabel, filterEnvelopeAmountSlider, containerA);
    placeDial (filterResonanceLabel, filterResonanceSlider, containerA);

    placeDial (ampAttackLabel, ampAttackSlider, containerB);
    placeDial (ampDecayLabel, ampDecaySlider, containerB);
    placeDial (ampSustainLabel, ampSustainSlider, containerB);
    placeDial (ampReleaseLabel, ampReleaseSlider, containerB);
}

void PluginEditor::handleNoteOn (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
    juce::MidiMessage message = juce::MidiMessage::noteOn (midiChannel, midiNoteNumber, velocity);
    processorRef.getMidiMessages().addEvent (message, 0);
}

void PluginEditor::handleNoteOff (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float)
{
    juce::MidiMessage message = juce::MidiMessage::noteOff (midiChannel, midiNoteNumber);
    processorRef.getMidiMessages().addEvent (message, 0);
}
