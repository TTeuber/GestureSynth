//
// Created by Tyler Teuber on 5/10/25.
//

#pragma once

#include "Utility/DualParameterComponent.h"

class SubOscillatorComponent : public DualParameterComponent
{
public:
    SubOscillatorComponent (juce::RangedAudioParameter* subOscParam,
        juce::RangedAudioParameter* subOscWaveParam)
        : DualParameterComponent (subOscParam, subOscWaveParam)
    {
        // Ensure we're working with the correct parameters
        jassert (subOscParam->getName (15).contains ("Sub") && subOscWaveParam->getName (15).contains ("Wave"));
    }

    SubOscillatorComponent (juce::AudioProcessorValueTreeState& apvts,
        juce::UndoManager* undoManager = nullptr,
        std::atomic<int>* gestureCount = nullptr,
        ModulationModeState* modModeState = nullptr)
        : DualParameterComponent (
              apvts.getParameter ("subOsc"),
              apvts.getParameter ("subOscWave"),
              dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter ("subOn")),
              undoManager,
              gestureCount,
              modModeState)
    {
    }

    ~SubOscillatorComponent() override = default;

protected:
    void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const override
    {
        // Get the current wave type (1-4)
        int waveType = static_cast<int> (std::round (param2Value * 3.0f)) + 1; // Map 0-1 to 1-4

        // Get the current amplitude (0-1)
        float amplitude = param1Value;

        // Set up the path
        juce::Path wavePath;

        // Start at left side, middle height
        float centerY = bounds.getCentreY();
        wavePath.startNewSubPath (bounds.getX(), centerY);

        // Draw the waveform
        drawWaveform (wavePath, bounds, waveType, amplitude, centerY);

        // Draw the path
        // g.setColour (TEXT_COLOR);
        g.strokePath (wavePath, juce::PathStrokeType (2.0f));
    }

    juce::String getParam2Text() const override
    {
        const float wave = param2->convertFrom0to1 (param2Value);
        if (wave == 1)
            return "Wave: Sine";
        if (wave == 2)
            return "Wave: Triangle";
        if (wave == 3)
            return "Wave: Square";
        if (wave == 4)
            return "Wave: Saw";

        return "Error";
    };

private:
    static void drawWaveform (juce::Path& path, const juce::Rectangle<int>& bounds, int waveType, float amplitude, float centerY)
    {
        const float width = bounds.getWidth();
        const float maxHeight = bounds.getHeight() / 2.0f * amplitude;

        switch (waveType)
        {
            case 1: // Sine
                for (int x = 0; x <= width; x += 2)
                {
                    float normalizedX = static_cast<float> (x) / width;
                    float y = centerY - maxHeight * std::sin (normalizedX * juce::MathConstants<float>::twoPi);
                    path.lineTo (bounds.getX() + x, y);
                }
                break;

            case 2: // Triangle
            {
                // First quarter (rising)
                path.lineTo (bounds.getX() + width * 0.25f, centerY - maxHeight);

                // Second and third quarter (falling)
                path.lineTo (bounds.getX() + width * 0.75f, centerY + maxHeight);

                // Fourth quarter (rising back to center)
                path.lineTo (bounds.getX() + width, centerY);
            }
            break;

            case 3: // Square
            {
                // First half (high)
                path.lineTo (bounds.getX(), centerY - maxHeight);
                path.lineTo (bounds.getX() + width * 0.5f, centerY - maxHeight);

                // Second half (low)
                path.lineTo (bounds.getX() + width * 0.5f, centerY + maxHeight);
                path.lineTo (bounds.getX() + width, centerY + maxHeight);
                path.lineTo (bounds.getX() + width, centerY);
            }
            break;

            case 4: // Saw
            {
                // Rising edge
                path.lineTo (bounds.getX(), centerY + maxHeight);

                // Falling edge (diagonal line from top to bottom)
                path.lineTo (bounds.getX() + width, centerY - maxHeight);

                // Back to center
                path.lineTo (bounds.getX() + width, centerY);
            }
            break;

            default:
            {
                jassertfalse;
                break;
            }
        }
    }
};