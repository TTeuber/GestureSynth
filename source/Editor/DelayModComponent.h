//
// Created by Tyler Teuber on 3/7/26.
//

#pragma once

#include "Utility/DualParameterComponent.h"
#include "../Utility/Parameters.h"

class DelayModComponent final : public DualParameterComponent
{
public:
    DelayModComponent (const juce::AudioProcessorValueTreeState& apvts,
        const UIContext& ctx = {},
        const juce::String& param1DestID = {},
        const juce::String& param2DestID = {})
        : DualParameterComponent (
              apvts.getParameter (ParamIDs::delayModDepth),
              apvts.getParameter (ParamIDs::delayModRate),
              dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (ParamIDs::delayOn)),
              ctx,
              param1DestID,
              param2DestID,
              "Modulation")
    {
    }

protected:
    void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const override
    {
        g.setOpacity (isActive ? 1.0f : 0.5f);
        g.setColour (getDrawColor());

        const float wavelength = bounds.getWidth() * (1.0f - param2Value * 0.8f);
        const float amplitude = bounds.getHeight() * 0.40f * param1Value;
        drawSineWave (g, bounds, amplitude, wavelength, 0.0f, 1.5f);
    }

    void drawVisualizationWithValues (juce::Graphics& g,
        const juce::Rectangle<int>& bounds, float p1, float p2) const override
    {
        const float wavelength = bounds.getWidth() * (1.0f - p2 * 0.8f);
        const float amplitude = bounds.getHeight() * 0.40f * p1;
        drawSineWave (g, bounds, amplitude, wavelength, 0.0f, 1.5f);
    }

    juce::String getParam1Text() const override
    {
        return formatParameterText (param1, param1Value, "{name}: {value}%");
    }

    juce::String getParam2Text() const override
    {
        return formatParameterText (param2, param2Value, "{name}: {value}%");
    }
private:
    static void drawSineWave (const juce::Graphics& g,
        const juce::Rectangle<int>& bounds,
        const float amplitude,
        const float wavelength,
        const float phaseOffset,
        const float strokeThickness)
    {
        juce::Path path;
        const float midY = bounds.getCentreY();

        const float step = juce::jmin (5.0f, wavelength / 20.0f);
        for (float x = 0; x <= bounds.getWidth(); x += step)
        {
            const float normalizedX = (x / wavelength) * juce::MathConstants<float>::twoPi;
            const float y = midY - amplitude * std::sin (normalizedX + phaseOffset * juce::MathConstants<float>::twoPi);

            if (x == 0)
                path.startNewSubPath (bounds.getX() + x, y);
            else
                path.lineTo (bounds.getX() + x, y);
        }

        g.strokePath (path, juce::PathStrokeType (strokeThickness));
    }
};
