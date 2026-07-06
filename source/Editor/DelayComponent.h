//
// Created by Tyler Teuber on 3/7/26.
//

#pragma once

#include "Utility/DualParameterComponent.h"
#include "../Utility/Parameters.h"

class DelayComponent final : public DualParameterComponent
{
public:
    DelayComponent (const juce::AudioProcessorValueTreeState& apvts,
        const UIContext& ctx = {},
        const juce::String& param1DestIDToUse = {},
        const juce::String& param2DestIDToUse = {})
        : DualParameterComponent (
              apvts.getParameter (ParamIDs::delayMix),
              apvts.getParameter (ParamIDs::delayFeedback),
              dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (ParamIDs::delayOn)),
              ctx,
              param1DestIDToUse,
              param2DestIDToUse,
              "Delay")
    {
    }

protected:
    void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const override
    {
        g.setOpacity (isActive ? 1.0f : 0.5f);
        g.setColour (getDrawColor());
        drawDelayArcs (g, bounds, param1Value, param2Value);
    }

    void drawVisualizationWithValues (juce::Graphics& g,
        const juce::Rectangle<int>& bounds, float p1, float p2) const override
    {
        drawDelayArcs (g, bounds, p1, p2);
    }

private:
    static void drawDelayArcs (juce::Graphics& g, const juce::Rectangle<int>& bounds,
        float mix, float feedback)
    {
        const float numArcsF = 3.0f + 4.0f * feedback;
        const int numArcs = static_cast<int> (std::ceil (numArcsF));
        const float width = static_cast<float> (bounds.getWidth());
        const float height = static_cast<float> (bounds.getHeight());
        const float left = static_cast<float> (bounds.getX()) - width * 0.08f;
        const float usableWidth = width * 1.08f;
        const float centerY = static_cast<float> (bounds.getY()) + height * 0.5f;
        const float baseHeight = height * 0.8f;
        const float decayRate = 1.2f - 0.9f * feedback;
        const float spacing = usableWidth / (numArcsF + 1.0f);
        const float baseThickness = 3.0f;

        for (int i = 0; i < numArcs; ++i)
        {
            const float fi = static_cast<float> (i);
            const float fractional = juce::jlimit (0.0f, 1.0f, numArcsF - fi);
            const float decayFactor = std::exp (-decayRate * fi);
            const float arcX = left + spacing * (fi + 1.0f);
            const float arcHeight = baseHeight * mix * decayFactor * fractional;
            const float halfHeight = arcHeight * 0.5f;
            const float curveDepth = spacing * 0.4f * std::max (0.3f, decayFactor);
            const float thickness = std::max (1.0f, baseThickness * std::exp (-0.3f * fi));

            juce::Path arc;
            arc.startNewSubPath (arcX, centerY - halfHeight);
            arc.quadraticTo (arcX + curveDepth, centerY, arcX, centerY + halfHeight);

            g.strokePath (arc, juce::PathStrokeType (thickness, juce::PathStrokeType::curved,
                juce::PathStrokeType::rounded));
        }
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
