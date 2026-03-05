//
// Created by Tyler Teuber on 5/10/25.
//

#pragma once

#include "Utility/DualParameterComponent.h"
#include "../Utility/Parameters.h"

class ChorusComponent final : public DualParameterComponent, private juce::Timer
{
public:
    ChorusComponent (juce::RangedAudioParameter* depthParam,
        juce::RangedAudioParameter* rateParam,
        juce::AudioParameterBool* activeParam = nullptr)
        : DualParameterComponent (depthParam, rateParam, activeParam)
    {
        // Constructor implementation
    }

    ChorusComponent (const juce::AudioProcessorValueTreeState& apvts,
        juce::UndoManager* undoManager = nullptr,
        std::atomic<int>* gestureCount = nullptr,
        std::atomic<float>* modDepthOutput = nullptr,
        std::atomic<float>* modRateOutput = nullptr,
        ModulationModeState* modModeState = nullptr,
        const juce::String& param1DestID = {},
        const juce::String& param2DestID = {})
        : DualParameterComponent (
              apvts.getParameter (ParamIDs::chorusDepth),
              apvts.getParameter (ParamIDs::chorusRate),
              dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (ParamIDs::chorusOn)),
              undoManager,
              gestureCount,
              modModeState,
              param1DestID,
              param2DestID),
          modDepthOutput (modDepthOutput),
          modRateOutput (modRateOutput)
    {
        if (modDepthOutput != nullptr || modRateOutput != nullptr)
            startTimerHz (30);
    }

    ~ChorusComponent() override { stopTimer(); }

protected:
    void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const override
    {
        g.setOpacity (isActive ? 1.0f : 0.5f);

        // Draw ghost path at modulated values (behind main)
        if ((modDepthOutput != nullptr || modRateOutput != nullptr)
            && (std::abs (modulatedDepth - param1Value) > 0.001f
                || std::abs (modulatedRate - param2Value) > 0.001f))
        {
            g.setColour (TEXT_COLOR.withAlpha (0.25f));
            drawChorusWaves (g, bounds, modulatedDepth, modulatedRate);
        }

        g.setColour (TEXT_COLOR);
        drawChorusWaves (g, bounds, param1Value, param2Value);
    }

    void drawVisualizationWithValues (juce::Graphics& g,
        const juce::Rectangle<int>& bounds, float p1, float p2) const override
    {
        drawChorusWaves (g, bounds, p1, p2);
    }

private:
    std::atomic<float>* modDepthOutput = nullptr;
    std::atomic<float>* modRateOutput = nullptr;
    float modulatedDepth = 0.0f;
    float modulatedRate = 0.0f;

    void timerCallback() override
    {
        constexpr float alpha = 0.3f;
        if (modDepthOutput != nullptr)
        {
            float target = modDepthOutput->load (std::memory_order_relaxed);
            modulatedDepth += alpha * (target - modulatedDepth);
        }
        if (modRateOutput != nullptr)
        {
            float target = modRateOutput->load (std::memory_order_relaxed);
            modulatedRate += alpha * (target - modulatedRate);
        }
        repaint();
    }

    static void drawChorusWaves (juce::Graphics& g, const juce::Rectangle<int>& bounds,
        float depth, float rate)
    {
        constexpr float strokeThickness = 1.5f;

        // Calculate wavelength based on rate (higher rate = shorter wavelength)
        const float baseWaveLength = bounds.getWidth() * (1.0f - rate * 0.8f);

        // Calculate amplitude based on depth
        const float maxAmplitude = bounds.getHeight() * 0.40f;
        const float amplitude = maxAmplitude * depth;

        // Draw three sine waves with phase offsets
        constexpr float phase1 = 0.0f;
        constexpr float phase2 = -0.1f;
        constexpr float phase3 = 0.1f;

        drawSineWave (g, bounds, amplitude, baseWaveLength, phase1, strokeThickness);
        drawSineWave (g, bounds, amplitude, baseWaveLength, phase2, strokeThickness / 1.5f);
        drawSineWave (g, bounds, amplitude, baseWaveLength, phase3, strokeThickness / 2.0f);
    }

protected:
    juce::String getParam1Text() const override
    {
        return formatParameterText (param1, param1Value, juce::StringRef ("Chrs Depth: ") + juce::String (param1Value * 30, param1Value == 1 ? 0 : 2) + juce::StringRef ("ms"));
    }

    juce::String getParam2Text() const override
    {
        return formatParameterText (param2, param2Value, juce::StringRef ("Chrs Rate: ") + juce::String (param2Value, param2Value == 1 ? 0 : 2) + juce::StringRef ("Hz"));
    }

private:
    static void drawSineWave (const juce::Graphics& g,
        const juce::Rectangle<int>& bounds,
        const float amplitude,
        const float wavelength,
        const float phaseOffset,
        const float strokeThickness)
    {
        // Create a path for the sine wave
        juce::Path path;

        // Calculate the midpoint of the bounds for y-axis
        const float midY = bounds.getCentreY();

        // Draw sine wave from left to right
        for (float x = 0; x <= bounds.getWidth(); x += 5.0f)
        {
            // Calculate normalized x-position (0 to 2π)
            const float normalizedX = (x / wavelength) * juce::MathConstants<float>::twoPi;

            // Calculate y-position using sine function with phase offset
            const float y = midY - amplitude * std::sin (normalizedX + phaseOffset * juce::MathConstants<float>::twoPi);

            if (x == 0)
            {
                // Start a new path on the left side of the bounds
                path.startNewSubPath (bounds.getX() + x, y);
            }
            else
            {
                // Add a point to a path
                path.lineTo (bounds.getX() + x, y);
            }
        }

        // Stroke the path
        g.strokePath (path, juce::PathStrokeType (strokeThickness));
    }
};