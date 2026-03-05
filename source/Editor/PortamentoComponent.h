//
// Created by Tyler Teuber
//

#pragma once

#include "Utility/SingleParameterComponent.h"

class PortamentoComponent final : public SingleParameterComponent
{
public:
    explicit PortamentoComponent (juce::RangedAudioParameter* portamentoParam,
        const UIContext& ctx = {})
        : SingleParameterComponent (portamentoParam, nullptr, ctx)
    {
    }

protected:
    void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const override
    {
        const float w = static_cast<float> (bounds.getWidth());
        const float h = static_cast<float> (bounds.getHeight()) * 0.6f;
        const float x0 = static_cast<float> (bounds.getX());
        const float y0 = static_cast<float> (bounds.getY()) * 1.2f;

        // k controls sigmoid steepness: near-vertical at paramValue=0, gentle slope at paramValue=1
        const float k = 40.0f - paramValue * 38.0f;

        // Draw 3 S-curves from bottom-left to top-right
        for (int line = 0; line < 3; ++line)
        {
            const float offset = (static_cast<float> (line) - 1.0f) * (h * 0.2f);

            juce::Path path;
            constexpr int numPoints = 128;
            for (int i = 0; i <= numPoints; ++i)
            {
                const float t = static_cast<float> (i) / static_cast<float> (numPoints);
                const float sigmoid = 1.0f / (1.0f + std::exp (-k * (t - 0.5f)));
                const float px = x0 + t * w;
                const float py = y0 + h - sigmoid * h + offset;

                if (i == 0)
                    path.startNewSubPath (px, py);
                else
                    path.lineTo (px, py);
            }

            g.strokePath (path, juce::PathStrokeType (1.5f));
        }
    }

    juce::String getParameterText() const override
    {
        const float ms = param->convertFrom0to1 (paramValue);
        if (ms < 1.0f)
            return "0 ms";
        return juce::String (static_cast<int> (ms)) + " ms";
    }
};
