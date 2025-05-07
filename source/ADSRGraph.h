#pragma once

#include "MyADSR.h"
#include "Theme.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

#include <utility>

class ADSRGraph final : public juce::Component, public juce::AudioProcessorValueTreeState::Listener, public juce::Timer
{
public:
    ADSRGraph (juce::AudioProcessorValueTreeState& p,
        juce::StringRef attackParam,
        juce::StringRef attackCurveParam,
        juce::StringRef decayParam,
        juce::StringRef decayCurveParam,
        juce::StringRef sustainParam,
        juce::StringRef releaseParam,
        juce::StringRef releaseCurveParam,
        std::shared_ptr<MyADSR*> adsr);
    ~ADSRGraph() override;
    void timerCallback() override;
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void setParameters (float attack, float decay, float sustain, float release);
    void resized() override;
    void paint (juce::Graphics& g) override;
    void drawCurve (juce::Graphics& g) const;
    void showTime();
    void mouseMove (const juce::MouseEvent& event) override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void mouseMagnify (const juce::MouseEvent& event, float scaleFactor) override;

    void setADSRPointer (std::shared_ptr<MyADSR*> adsr) { myADSR = std::move (adsr); }

private:
    float getCurveY (float x) const;

    juce::AudioProcessorValueTreeState& parameters;

    float width;
    float height;

    juce::String attackId;
    juce::String attackCurveId;
    juce::String decayId;
    juce::String decayCurveId;
    juce::String sustainId;
    juce::String releaseId;
    juce::String releaseCurveId;

    juce::Point<float> attackPoint;
    juce::Point<float> attackCurvePoint;
    juce::Point<float> decayPoint;
    juce::Point<float> decayCurvePoint;
    juce::Point<float> releasePoint;
    juce::Point<float> releaseCurvePoint;
    juce::Point<float> clickPoint;

    float attackX = 0.0f;
    float decayX = 0.0f;
    float releaseX = 0.0f;

    std::vector<juce::Point<float>*> points = {
        &attackPoint,
        &attackCurvePoint,
        &decayPoint,
        &decayCurvePoint,
        &releasePoint,
        &releaseCurvePoint,
    };

    juce::Point<float> timePoint;

    float durationWidth = 3.2f;
    float xOffset = 0.0f;

    enum Point {
        Attack,
        AttackCurve,
        Decay,
        DecayCurve,
        Sustain,
        Release,
        ReleaseCurve,
        None
    };
    Point selectedPoint;

    float sustain;

    float attackTime;
    float attackCurve;
    float decayTime;
    float decayCurve;
    float sustainLevel = -1.0f;
    float releaseTime;
    float releaseCurve;
    float totalDuration = 1.0f;

    std::shared_ptr<MyADSR*> myADSR;
};
