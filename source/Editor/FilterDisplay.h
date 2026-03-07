//
// Created by Tyler Teuber on 4/27/25.
//

#pragma once

#include "../Theme.h"
#include "Utility/AnimationFrameSource.h"
#include "Utility/ModulationModeState.h"
#include "Utility/ModulationContextMenu.h"
#include "Utility/UIContext.h"

#include <juce_audio_processors/juce_audio_processors.h>

//==============================================================================
class FilterDisplay final : public juce::Component,
                            juce::AudioProcessorValueTreeState::Listener,
                            public AnimationFrameSource::Listener,
                            private juce::Timer
{
public:
    explicit FilterDisplay (juce::AudioProcessorValueTreeState& apvts,
        const UIContext& ctx = {},
        std::atomic<float>* modCutoffOutput = nullptr,
        std::atomic<float>* modResonanceOutput = nullptr);

    ~FilterDisplay() override;

    void paint (juce::Graphics& g) override;

    void resized() override;

    void mouseDown (const juce::MouseEvent& e) override;

    void mouseDrag (const juce::MouseEvent& e) override;

    void mouseUp (const juce::MouseEvent& e) override;

    void mouseEnter (const juce::MouseEvent&) override;

    void mouseExit (const juce::MouseEvent&) override;

private:
    float hoverAlpha = 0.0f;
    bool hoverTarget = false;
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

    juce::Path filterResponsePath;
    bool isDragging = false;

    // For fine control
    juce::Point<float> dragStartPosition;
    float dragStartCutoff = 0.5f;
    float dragStartResonance = 0.5f;

    // Display frequency mapping (true logarithmic, x=0 -> 20Hz, x=1 -> 20kHz)
    static constexpr double kDisplayMinFreq = 20.0;
    static constexpr double kDisplayMaxFreq = 20000.0;

    double displayXToFreq (double x) const;
    double freqToDisplayX (double freq) const;

    //==============================================================================
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void updateParameterValues();

    void drawFrequencyPath (juce::Graphics& g) const;

    // Computes the gain in dB for a given x-coordinate (0 to 1), cutoff frequency, Q, and filter order
    double getYCoordinate (const float x, const int order = 2) const;

    // Parameterized helper for drawing a filter curve at arbitrary cutoff/resonance
    void drawFilterCurveAt (juce::Graphics& g, float normCutoff, float normRes,
        juce::Colour colour, float strokeWidth) const;

    // Computes the gain in dB for a single second-order low-pass filter stage
    static double computeSecondOrderStage (const double freq, const double cutoffFreq, const double q);

    // Computes the gain in dB for a single first-order low-pass filter stage
    static double computeFirstOrderStage (const double freq, const double cutoffFreq);

    void drawParameterValues (juce::Graphics& g) const;

    juce::UndoManager* undoManager = nullptr;
    std::atomic<int>* gestureCount = nullptr;

    // Modulation ghost curve
    std::atomic<float>* modCutoffOutput = nullptr;
    std::atomic<float>* modResonanceOutput = nullptr;
    float modulatedNormCutoff = 0.5f;
    float modulatedNormResonance = 0.5f;

    void timerCallback() override;
    void onAnimationFrame() override;
    void drawModulatedFrequencyPath (juce::Graphics& g) const;
    AnimationFrameSource* animSource = nullptr;

    // Modulation mode
    ModulationModeState* modModeState = nullptr;
    bool isModDragging = false;
    float modDragInitialCutoffDepth = 0.0f;
    float modDragInitialResDepth = 0.0f;
    int modDragStartX = 0;
    int modDragStartY = 0;

    void drawModModeOverlay (juce::Graphics& g) const;

    // Axis-locking dead zone (normal / shift+mod drag)
    bool cutoffEngaged = false;
    bool resonanceEngaged = false;
    int cutoffRefX = 0;
    int resonanceRefY = 0;
    float initialCutoffValue = 0.0f;
    float initialResonanceValue = 0.0f;

    // Axis-locking dead zone (mod drag)
    bool modCutoffEngaged = false;
    bool modResonanceEngaged = false;
    int modCutoffRefX = 0;
    int modResonanceRefY = 0;

    static constexpr int kPrimaryThreshold = 4;
    static constexpr int kSecondaryThreshold = 12;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterDisplay)
};
