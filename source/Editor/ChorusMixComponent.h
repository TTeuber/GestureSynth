//
// Created by Tyler Teuber
//

#pragma once

#include "Utility/SingleParameterComponent.h"

class ChorusMixComponent final : public SingleParameterComponent
{
public:
    ChorusMixComponent (juce::RangedAudioParameter* chorusMixParam,
        juce::AudioParameterBool* activeParam = nullptr,
        const UIContext& ctx = {})
        : SingleParameterComponent (chorusMixParam, activeParam, ctx)
    {
    }

protected:
    void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const override
    {
        const float cx = bounds.getCentreX();
        const float cy = bounds.getCentreY();
        const float dim = static_cast<float> (juce::jmin (bounds.getWidth(), bounds.getHeight()));
        const float dryRadius = dim * 0.4f;
        const float wetRadius = dryRadius * 0.6f;
        const float offset = wetRadius * 0.6f;

        const float activeAlpha = isActive ? 1.0f : Style::alphaInactive;

        // Single circle (dry signal) — fades out as mix increases
        g.setOpacity ((1.0f - paramValue) * activeAlpha);
        g.drawEllipse (cx - dryRadius, cy - dryRadius, dryRadius * 2.0f, dryRadius * 2.0f, 2.0f);

        // Three intersecting circles (wet/chorus signal) — fades in as mix increases
        g.setOpacity (paramValue * activeAlpha);
        constexpr float angles[] = {
            -juce::MathConstants<float>::halfPi,                          // 90° (top)
            -juce::MathConstants<float>::halfPi + juce::MathConstants<float>::twoPi / 3.0f,  // 210°
            -juce::MathConstants<float>::halfPi + juce::MathConstants<float>::twoPi * 2.0f / 3.0f  // 330°
        };

        for (const float angle : angles)
        {
            const float x = cx + offset * std::cos (angle);
            const float y = cy + offset * std::sin (angle);
            g.drawEllipse (x - wetRadius, y - wetRadius, wetRadius * 2.0f, wetRadius * 2.0f, 2.0f);
        }

        g.setOpacity (1.0f);
    }
};
