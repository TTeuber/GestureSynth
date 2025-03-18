#pragma once

#include "MyADSR.h"

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <utility>

class ADSRGraph final : public juce::Component, juce::AudioProcessorValueTreeState::Listener, juce::Timer
{
public:
    ADSRGraph (juce::AudioProcessorValueTreeState& p,
        juce::StringRef attackParam,
        juce::StringRef decayParam,
        juce::StringRef sustainParam,
        juce::StringRef releaseParam,
        std::shared_ptr<MyADSR*> adsr);
    ~ADSRGraph() override;
    void timerCallback() override;
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void setParameters (float attack, float decay, float sustain, float release);
    void paint (juce::Graphics& g) override;
    void drawCurve (juce::Graphics& g, int attackX, int decayX, int releaseX) const;
    float getCurveY (float x, int width, int height, int attackX, int decayX, int releaseX) const;
    void showTime();
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void mouseMagnify (const juce::MouseEvent& event, float scaleFactor) override;

    void setADSRPointer (std::shared_ptr<MyADSR*> adsr) { myADSR = std::move (adsr); }

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

    juce::Point<float> timePoint;

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

    std::shared_ptr<MyADSR*> myADSR;
};
