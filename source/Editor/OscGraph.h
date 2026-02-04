//
// Created by Tyler Teuber on 3/19/25.
//

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Utility/DualParameterComponent.h"

// First Derived Class (WaveformComponent)
class WaveformComponent final : public DualParameterComponent
{
public:
    WaveformComponent (juce::RangedAudioParameter* waveformParam,
        juce::RangedAudioParameter* pulseWidthParam)
        : DualParameterComponent (waveformParam, pulseWidthParam)
    {
    }

    WaveformComponent (juce::AudioProcessorValueTreeState& apvts)
        : DualParameterComponent (
              apvts.getParameter ("oscWaveform"),
              apvts.getParameter ("pulseWidth"),
              dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter ("oscOn")))
    {
    }

protected:
    void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const override
    {
        const float centerY = bounds.getCentreY();
        drawPath (g, bounds, centerY, 0.0f, param1Value);
    }

private:
    void drawPath (juce::Graphics& g, const juce::Rectangle<int>& bounds, const float centerY, const float xOffset, const float waveformMix) const
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
        const float topPulseY = topLeft + (topSlope * width * param2Value);
        const float bottomPulseY = bottomRight - (bottomSlope * width * (1.0f - param2Value));

        // Draw the path
        path.startNewSubPath (startX, topLeft);
        path.lineTo (startX + width * param2Value, topPulseY);
        path.lineTo (startX + width * param2Value, bottomPulseY);
        path.lineTo (startX + width, bottomRight);

        // Draw the path
        g.strokePath (path, juce::PathStrokeType (2.0f));
    }
};