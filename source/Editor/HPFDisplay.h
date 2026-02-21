//
// High Pass Filter Display Component
//

#pragma once

#include "../Theme.h"

#include <juce_audio_processors/juce_audio_processors.h>

//==============================================================================
class HPFDisplay final : public juce::Component,
                         juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit HPFDisplay (juce::AudioProcessorValueTreeState& apvts);

    ~HPFDisplay() override;

    void paint (juce::Graphics& g) override;

    void resized() override;

    void mouseDown (const juce::MouseEvent& e) override;

    void mouseDrag (const juce::MouseEvent& e) override;

    void mouseUp (const juce::MouseEvent& e) override;

private:
    juce::AudioProcessorValueTreeState& apvts;

    juce::RangedAudioParameter* cutoffParam = nullptr;

    // Normalized parameter value (0-1)
    float normalizedCutoff = 0.0f;

    bool hpfEnabled = true;

    // Actual parameter value
    float cutoffFrequency = 20.0f;

    // For control point
    juce::Point<float> controlPoint;
    const float controlPointRadius = 8.0f;
    bool isDragging = false;

    // For fine control
    juce::Point<float> dragStartPosition;
    float dragStartDisplayX = 0.0f;

    // Display frequency mapping (x=0 → 20Hz, x=1 → 3kHz)
    static constexpr double kDisplayMinFreq = 20.0;
    static constexpr double kDisplayMaxFreq = 3000.0;
    // Curve starts drawing from here (maps to negative pixels) for visible rolloff
    static constexpr double kCurveStartFreq = 5.0;

    double displayXToFreq (double x) const;
    double freqToDisplayX (double freq) const;

    //==============================================================================
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void updateParameterValues();

    void drawFrequencyPath (juce::Graphics& g) const;

    // Computes the gain in dB for a given frequency using HP transfer function
    double getHPGainDb (double freq) const;

    void updateControlPointPosition();

    bool isMouseOverControlPoint (const juce::Point<int>& mousePosition) const;

    void drawControlPoint (juce::Graphics& g) const;

    void drawParameterValues (juce::Graphics& g) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HPFDisplay)
};
