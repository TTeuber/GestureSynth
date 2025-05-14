//
// Created by Tyler Teuber on 5/10/25.
//

#pragma once

#include "DualParameterComponent.h"

class ChorusComponent final : public DualParameterComponent
{
public:
    ChorusComponent (juce::RangedAudioParameter* depthParam,
        juce::RangedAudioParameter* rateParam,
        juce::AudioParameterBool* activeParam = nullptr)
        : DualParameterComponent (depthParam, rateParam, activeParam)
    {
        // Constructor implementation
    }

    explicit ChorusComponent (const juce::AudioProcessorValueTreeState& apvts)
        : DualParameterComponent (
              apvts.getParameter ("chorusDepth"),
              apvts.getParameter ("chorusRate"),
              dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter ("chorusOn")))
    {
    }

    ~ChorusComponent() override = default;

protected:
    void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const override
    {
        // Set up colors and stroke thickness
        g.setColour (TEXT_COLOR);
        g.setOpacity (isActive ? 1.0f : 0.5f);

        constexpr float strokeThickness = 1.5f;

        // Calculate parameters for the sine waves
        const float depth = param1Value; // chorusDepth - controls amplitude
        const float rate = param2Value; // chorusRate - controls wavelength

        // Calculate wavelength based on rate (higher rate = shorter wavelength)
        const float baseWaveLength = bounds.getWidth() * (1.0f - rate * 0.8f); // Invert rate effect

        // Calculate amplitude based on depth
        const float maxAmplitude = bounds.getHeight() * 0.40f;
        const float amplitude = maxAmplitude * depth;

        // Draw three sine waves with phase offsets
        constexpr float phase1 = 0.0f;
        constexpr float phase2 = -0.1f; // Slight phase offset for the second wave
        constexpr float phase3 = 0.1f; // Larger phase offset for the third wave

        drawSineWave (g, bounds, amplitude, baseWaveLength, phase1, strokeThickness);
        drawSineWave (g, bounds, amplitude, baseWaveLength, phase2, strokeThickness / 1.5);
        drawSineWave (g, bounds, amplitude, baseWaveLength, phase3, strokeThickness / 2);
    }

    juce::String getParam1Text() const override
    {
        return formatParameterText (param1, param1Value, juce::StringRef ("Chrs Depth: ") + juce::String (param1Value * 10, param1Value == 1 ? 0 : 2) + juce::StringRef ("ms"));
    }

    juce::String getParam2Text() const override
    {
        return formatParameterText (param2, param2Value, juce::StringRef ("Chrs Rate: ") + juce::String (param2Value * 10, param2Value == 1 ? 0 : 2) + juce::StringRef ("Hz"));
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