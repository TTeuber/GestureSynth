//
// Created by Tyler Teuber on 3/19/25.
//

#pragma once

#include "Theme.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

class WaveformComponent final : public juce::Component,
                                private juce::AudioProcessorValueTreeState::Listener
{
public:
    WaveformComponent (juce::AudioProcessorValueTreeState& apvts,
        const juce::StringRef waveformParamId,
        const juce::StringRef pulseWidthParamId,
        const juce::StringRef detuneParamId)
        : apvts (apvts),
          waveformParamId (waveformParamId),
          pulseWidthParamId (pulseWidthParamId),
          detuneParamId (detuneParamId)
    {
        // Register as a listener for both parameters
        apvts.addParameterListener (waveformParamId, this);
        apvts.addParameterListener (pulseWidthParamId, this);
        apvts.addParameterListener (detuneParamId, this);

        // Set initial values from parameters
        waveformValue = apvts.getRawParameterValue (waveformParamId)->load();
        pulseWidthValue = apvts.getRawParameterValue (pulseWidthParamId)->load();
        detuneValue = apvts.getRawParameterValue (detuneParamId)->load();
    }

    ~WaveformComponent() override
    {
        // Remove listeners
        apvts.removeParameterListener (waveformParamId, this);
        apvts.removeParameterListener (detuneParamId, this);
    }

    void paint (juce::Graphics& g) override
    {
        // Draw background
        g.fillAll (SECONDARY_COLOR);

        // Calculate dimensions for the waveform
        const auto bounds = getLocalBounds().reduced (20);
        const float width = bounds.getWidth();
        // float height = bounds.getHeight();
        const float centerY = bounds.getCentreY();

        // Draw main waveform
        g.setColour (TEXT_COLOR);
        drawWaveform (g, bounds, centerY, 0.0f, waveformValue);

        // Draw detune shadow waveform if detune > 0
        if (detuneValue > 0.0f)
        {
            g.setColour (SECONDARY_COLOR);

            // Draw left and right detune shadows
            float detuneOffset = detuneValue * width * 0.1f; // Scale detune effect
            drawWaveform (g, bounds, centerY, -detuneOffset, waveformValue);
            // No need for a right shadow as the main waveform will serve as the right side
        }

        // Draw percentage text indicators
        g.setColour (juce::Colours::white);
        g.setFont (14.0f);

        // Display waveform percentage (bottom left)
        int waveformPercentage = static_cast<int> (waveformValue * 100.0f);
        g.drawText ("waveform: " + juce::String (waveformPercentage) + "%",
            10,
            getHeight() - 25,
            120,
            20,
            juce::Justification::bottomLeft,
            true);

        // Display detune percentage (top right)
        int detunePercentage = static_cast<int> (detuneValue * 100.0f);
        g.drawText ("detune: " + juce::String (detunePercentage) + "%",
            getWidth() - 120,
            5,
            110,
            20,
            juce::Justification::topRight,
            true);
    }

    void resized() override
    {
        // Ensure square aspect ratio
        auto size = juce::jmin (getWidth(), getHeight());
        setBounds (getX(), getY(), size, size);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        // Store initial mouse position and parameter values for drag
        mouseDownX = e.x;
        mouseDownY = e.y;
        initialWaveformValue = waveformValue;
        initialPulseWidthValue = pulseWidthValue;
        initialDetuneValue = detuneValue;

        isDragging = true;
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (!isDragging)
            return;

        const auto bounds = getLocalBounds();

        // Calculate vertical movement for waveform parameter (up = more square, down = more saw)
        const float verticalDelta = (mouseDownY - e.y) / static_cast<float> (bounds.getHeight() - padding);
        const float newWaveformValue = juce::jlimit (0.0f, 1.0f, initialWaveformValue + verticalDelta);

        // Calculate horizontal movement for pulse width parameter (right = more width)
        const float horizontalDelta = (e.x - mouseDownX) / static_cast<float> (bounds.getWidth() - padding);
        const float newPulseValue = juce::jlimit (0.0f, 1.0f, initialPulseWidthValue + horizontalDelta);

        // Update parameters in the AudioProcessorValueTreeState
        apvts.getParameter (waveformParamId)->setValueNotifyingHost (newWaveformValue);
        apvts.getParameter (pulseWidthParamId)->setValueNotifyingHost (newPulseValue);
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        isDragging = false;
    }

private:
    void parameterChanged (const juce::String& parameterID, const float newValue) override
    {
        // Update local values when parameters change
        if (parameterID == waveformParamId)
            waveformValue = newValue;
        else if (parameterID == detuneParamId)
            detuneValue = newValue;
        else if (parameterID == pulseWidthParamId)
            pulseWidthValue = newValue;

        repaint();
    }

    void drawWaveform (juce::Graphics& g, const juce::Rectangle<int>& bounds, const float centerY, const float xOffset, const float waveformMix) const
    {
        // Create waveform path
        juce::Path path;
        const float width = bounds.getWidth();
        const float startX = bounds.getX() + xOffset;

        const float amplitude = bounds.getHeight() * padding; // 40% of height for amplitude

        // Define fixed points for consistent slope calculation
        const float topLeft = centerY - amplitude;
        const float bottomRight = centerY + amplitude;

        // Calculate the reference slopes based on a reference pulse width (0.5)
        constexpr float referencePulseWidth = 0.5f;
        const float topSlope = ((centerY - amplitude * waveformMix) - topLeft) / (width * referencePulseWidth);
        const float bottomSlope = (bottomRight - (centerY + amplitude * waveformMix)) / (width * (1.0f - referencePulseWidth));

        // Calculate y positions at the pulse point that maintain these slopes
        const float topPulseY = topLeft + (topSlope * width * pulseWidthValue);
        const float bottomPulseY = bottomRight - (bottomSlope * width * (1.0f - pulseWidthValue));

        // Draw the path
        path.startNewSubPath (startX, topLeft);
        path.lineTo (startX + width * pulseWidthValue, topPulseY);
        path.lineTo (startX + width * pulseWidthValue, bottomPulseY);
        path.lineTo (startX + width, bottomRight);

        // Draw the path
        g.strokePath (path, juce::PathStrokeType (2.0f));
    }

    // Reference to the value tree
    juce::AudioProcessorValueTreeState& apvts;

    // Parameter IDs
    juce::String waveformParamId;
    juce::String pulseWidthParamId;
    juce::String detuneParamId;

    // Current parameter values
    float waveformValue = 0.0f;
    float pulseWidthValue = 0.0f;
    float detuneValue = 0.0f;

    float padding = 0.4f; // Padding for waveform drawing

    // Mouse drag tracking
    bool isDragging = false;
    int mouseDownX = 0;
    int mouseDownY = 0;
    float initialWaveformValue = 0.0f;
    float initialPulseWidthValue = 0.0f;
    float initialDetuneValue = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformComponent)
};
