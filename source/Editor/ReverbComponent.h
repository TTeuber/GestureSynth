//
// Created by Tyler Teuber on 3/7/26.
//

#pragma once

#include "Utility/DualParameterComponent.h"
#include "../Utility/Parameters.h"

class ReverbComponent final : public DualParameterComponent
{
public:
    ReverbComponent (const juce::AudioProcessorValueTreeState& apvts,
        const UIContext& ctx = {},
        const juce::String& param1DestIDToUse = {},
        const juce::String& param2DestIDToUse = {})
        : DualParameterComponent (
              apvts.getParameter (ParamIDs::reverbLevel),
              apvts.getParameter (ParamIDs::reverbDecay),
              dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (ParamIDs::reverbOn)),
              ctx,
              param1DestIDToUse,
              param2DestIDToUse,
              "Reverb")
    {
    }

protected:
    void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const override
    {
        g.setOpacity (isActive ? 1.0f : 0.5f);
        g.setColour (getDrawColor());
        drawDampedOscillation (g, bounds, param1Value, param2Value);
    }

    void drawVisualizationWithValues (juce::Graphics& g,
        const juce::Rectangle<int>& bounds, float p1, float p2) const override
    {
        drawDampedOscillation (g, bounds, p1, p2);
    }

private:
    static void drawDampedOscillation (juce::Graphics& g, const juce::Rectangle<int>& bounds,
        float mix, float decay)
    {
        juce::Path path;
        const float width = static_cast<float> (bounds.getWidth());
        const float height = static_cast<float> (bounds.getHeight());
        const float left = static_cast<float> (bounds.getX());
        const float top = static_cast<float> (bounds.getY());

        for (float px = 0; px <= width; px += 1.5f)
        {
            const float nx = px / width;
            const float y = std::exp ((-14.0f + 12.0f * decay) * nx)
                          * std::sin (20.0f * juce::MathConstants<float>::pi * nx)
                          * (mix - std::pow(mix, 2.0f) + std::pow(0.8f * mix, 3.0f)) + 0.5f;

            const float screenY = top + height * (1.0f - y);

            if (juce::exactlyEqual (px, 0.0f))
                path.startNewSubPath (left, screenY);
            else
                path.lineTo (left + px, screenY);
        }

        g.strokePath (path, juce::PathStrokeType (1.5f));
    }

    juce::String getParam1Text() const override
    {
        return formatParameterText (param1, param1Value, "{name}: {value}%");
    }

    juce::String getParam2Text() const override
    {
        return formatParameterText (param2, param2Value, "{name}: {value}%");
    }
};
