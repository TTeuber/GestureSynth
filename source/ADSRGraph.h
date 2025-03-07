#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

class ADSRGraph final : public juce::Component, juce::AudioProcessorValueTreeState::Listener
{
public:
    ADSRGraph (juce::AudioProcessorValueTreeState& p,
        juce::StringRef attackParam,
        juce::StringRef decayParam,
        juce::StringRef sustainParam,
        juce::StringRef releaseParam);
    ~ADSRGraph() override;
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void setParameters (float attack, float decay, float sustain, float release);
    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void mouseMagnify (const juce::MouseEvent& event, float scaleFactor) override;

private:
    juce::AudioProcessorValueTreeState& parameters;

    juce::String attackId;
    juce::String decayId;
    juce::String sustainId;
    juce::String releaseId;

    juce::Point<float> attackPoint;
    juce::Point<float> decayPoint;
    // juce::Point<float> sustainPoint;
    juce::Point<float> releasePoint;
    juce::Point<float> clickPoint;

    float durationWidth = 3.2f;
    float xOffset = 0.0f;

    enum Point {
        Attack,
        Decay,
        Sustain,
        Release,
        None
    };
    Point selectedPoint;

    float attackTime;
    float decayTime;
    float sustainLevel;
    float releaseTime;
    float totalDuration = 1.0f;
};
