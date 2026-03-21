//
// Created by Tyler Teuber on 3/21/26.
//

#pragma once

#include "Utility/SingleParameterComponent.h"

class ReverbPreDelayComponent final : public SingleParameterComponent
{
public:
    ReverbPreDelayComponent (juce::RangedAudioParameter* param,
        juce::AudioParameterBool* activeParam = nullptr,
        const UIContext& ctx = {},
        const juce::String& paramDestID = {})
        : SingleParameterComponent (param, activeParam, ctx, paramDestID)
    {
    }

protected:
    void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const override
    {
        g.setOpacity (isActive ? 1.0f : 0.5f);
        g.setColour (isActive ? TEXT_COLOR : TEXT_INACTIVE_COLOR);

        juce::Path path;
        const float width = static_cast<float> (bounds.getWidth());
        const float height = static_cast<float> (bounds.getHeight());
        const float left = static_cast<float> (bounds.getX());
        const float top = static_cast<float> (bounds.getY());
        const float centreY = top + height * 0.5f;

        // Flat section: horizontal line at vertical centre from x=0 to x=paramValue*width
        const float flatEnd = paramValue * width;

        path.startNewSubPath (left, centreY);
        path.lineTo (left + flatEnd, centreY);

        // Fixed-width damped oscillation that translates with paramValue
        constexpr float oscLength = 120.0f;
        constexpr float decay = 4.0f;
        constexpr float freq = 8.0f * juce::MathConstants<float>::pi;
        constexpr float amplitude = 0.45f;

        const float rightEdge = left + width;
        for (float px = 1.5f; px <= oscLength; px += 1.5f)
        {
            const float screenX = left + flatEnd + px;
            if (screenX > rightEdge)
                break;

            const float t = px / oscLength;
            const float y = std::exp (-decay * t) * std::sin (freq * t) * amplitude;
            const float screenY = centreY - y * height;
            path.lineTo (screenX, screenY);
        }

        g.strokePath (path, juce::PathStrokeType (1.5f));
    }

    juce::String getParameterText() const override
    {
        return formatParameterText (param, paramValue, "{value} {unit}");
    }
};
