//
// Created by Tyler Teuber on 4/27/25.
//

#pragma once

#include "Theme.h"

#include <juce_audio_processors/juce_audio_processors.h>

//==============================================================================
class FilterDisplay final : public juce::Component,
                            juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit FilterDisplay (juce::AudioProcessorValueTreeState& apvts);

    ~FilterDisplay() override;

    void paint (juce::Graphics& g) override;

    void resized() override;

    void mouseDown (const juce::MouseEvent& e) override;

    void mouseDrag (const juce::MouseEvent& e) override;

    void mouseUp (const juce::MouseEvent& e) override;

private:
    juce::AudioProcessorValueTreeState& apvts;

    juce::RangedAudioParameter* cutoffParam = nullptr;
    juce::RangedAudioParameter* resonanceParam = nullptr;

    // Normalized parameter values (0-1)
    float normalizedCutoff = 0.5f;
    float normalizedResonance = 0.5f;

    bool filterEnabled = true;

    // Actual parameter values
    float cutoffFrequency = 1000.0f;
    float resonance = 0.71f;

    // For control point
    juce::Point<float> controlPoint;
    juce::Path filterResponsePath;
    const float controlPointRadius = 8.0f;
    bool isDragging = false;

    // For fine control
    juce::Point<float> dragStartPosition;
    float dragStartCutoff = 0.5f;
    float dragStartResonance = 0.5f;

    //==============================================================================
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void updateParameterValues();

    void drawFrequencyPath (juce::Graphics& g) const;

    // Computes the gain in dB for a given x-coordinate (0 to 1), cutoff frequency, Q, and filter order
    double getYCoordinate (const float x, const int order = 2) const;

    // Computes the gain in dB for a single second-order low-pass filter stage
    static double computeSecondOrderStage (const double freq, const double cutoffFreq, const double q);

    // Computes the gain in dB for a single first-order low-pass filter stage
    static double computeFirstOrderStage (const double freq, const double cutoffFreq);

    void updateControlPointPosition();

    bool isMouseOverControlPoint (const juce::Point<int>& mousePosition) const;

    void drawControlPoint (juce::Graphics& g) const;

    void drawParameterValues (juce::Graphics& g) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterDisplay)
};
