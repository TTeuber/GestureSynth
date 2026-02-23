//
// Created by Tyler Teuber on 3/19/25.
//

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Utility/DualParameterComponent.h"

// First Derived Class (WaveformComponent)
class WaveformComponent final : public DualParameterComponent, private juce::Timer
{
public:
    WaveformComponent (juce::RangedAudioParameter* waveformParam,
        juce::RangedAudioParameter* pulseWidthParam)
        : DualParameterComponent (waveformParam, pulseWidthParam)
    {
    }

    WaveformComponent (juce::AudioProcessorValueTreeState& apvts,
        juce::UndoManager* undoManager = nullptr,
        std::atomic<int>* gestureCount = nullptr,
        std::atomic<float>* modPulseWidthOutput = nullptr)
        : DualParameterComponent (
              apvts.getParameter ("oscWaveform"),
              apvts.getParameter ("pulseWidth"),
              dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter ("oscOn")),
              undoManager,
              gestureCount),
          modPulseWidthOutput (modPulseWidthOutput)
    {
        if (modPulseWidthOutput != nullptr)
            startTimerHz (30);
    }

    ~WaveformComponent() override { stopTimer(); }

protected:
    void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const override
    {
        const float centerY = bounds.getCentreY();

        // Draw ghost path at modulated pulse width (behind main)
        if (modPulseWidthOutput != nullptr
            && std::abs (modulatedPulseWidth - param2Value) > 0.001f)
        {
            g.setColour (TEXT_COLOR.withAlpha (0.25f));
            drawPath (g, bounds, centerY, 0.0f, param1Value, modulatedPulseWidth);
        }

        // Draw main path on top
        g.setColour (TEXT_COLOR);
        drawPath (g, bounds, centerY, 0.0f, param1Value);
    }

private:
    std::atomic<float>* modPulseWidthOutput = nullptr;
    float modulatedPulseWidth = 0.5f;

    void timerCallback() override
    {
        if (modPulseWidthOutput != nullptr)
        {
            constexpr float alpha = 0.3f;
            float target = modPulseWidthOutput->load (std::memory_order_relaxed);
            modulatedPulseWidth += alpha * (target - modulatedPulseWidth);
        }
        repaint();
    }

    void drawPath (juce::Graphics& g, const juce::Rectangle<int>& bounds, const float centerY, const float xOffset, const float waveformMix, const float pulseWidth = -1.0f) const
    {
        const float pw = (pulseWidth >= 0.0f) ? pulseWidth : param2Value;

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
        const float topPulseY = topLeft + (topSlope * width * pw);
        const float bottomPulseY = bottomRight - (bottomSlope * width * (1.0f - pw));

        // Draw the path
        path.startNewSubPath (startX, topLeft);
        path.lineTo (startX + width * pw, topPulseY);
        path.lineTo (startX + width * pw, bottomPulseY);
        path.lineTo (startX + width, bottomRight);

        // Draw the path
        g.strokePath (path, juce::PathStrokeType (2.0f));
    }
};