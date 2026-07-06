//
// Created by Tyler Teuber
//

#pragma once

#include "Utility/SingleParameterComponent.h"

class ChorusMixComponent final : public SingleParameterComponent
{
public:
    ChorusMixComponent (juce::RangedAudioParameter* chorusMixParam,
        juce::AudioParameterBool* activeParamToUse = nullptr,
        const UIContext& ctx = {})
        : SingleParameterComponent (chorusMixParam, activeParamToUse, ctx)
    {
    }

protected:
    void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const override
    {
        const float cx = static_cast<float> (bounds.getCentreX());
        const float cy = static_cast<float> (bounds.getCentreY());
        const float dim = static_cast<float> (juce::jmin (bounds.getWidth(), bounds.getHeight()));
        const float radius = dim * 0.28f;
        const float maxOffset = radius * 0.6f;

        // Three voice circles: concentric at mix = 0 (reads as one dry circle),
        // sliding apart into a trefoil as the mix increases
        const float offset = paramValue * maxOffset;
        constexpr float angles[] = {
            -juce::MathConstants<float>::halfPi,                          // 90° (top)
            -juce::MathConstants<float>::halfPi + juce::MathConstants<float>::twoPi / 3.0f,  // 210°
            -juce::MathConstants<float>::halfPi + juce::MathConstants<float>::twoPi * 2.0f / 3.0f  // 330°
        };

        for (const float angle : angles)
        {
            const float x = cx + offset * std::cos (angle);
            const float y = cy + offset * std::sin (angle);
            g.drawEllipse (x - radius, y - radius, radius * 2.0f, radius * 2.0f, 2.0f);
        }
    }
};
