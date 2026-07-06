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
        const float w = static_cast<float> (bounds.getWidth());
        const float x = static_cast<float> (bounds.getX());
        const float cy = static_cast<float> (bounds.getCentreY());
        const float amplitude = static_cast<float> (bounds.getHeight()) * 0.30f;

        // Three voice waves: coincident at mix = 0 (reads as one dry wave),
        // fanning out in phase as the mix increases
        constexpr float cycles = 1.5f;
        constexpr float maxPhase = juce::MathConstants<float>::twoPi / 3.0f;

        for (int voice = -1; voice <= 1; ++voice)
        {
            const float phase = static_cast<float> (voice) * paramValue * maxPhase;

            juce::Path path;
            for (float px = 0.0f; px <= w; px += 2.0f)
            {
                const float t = px / w;
                const float y = cy - amplitude * std::sin (juce::MathConstants<float>::twoPi * cycles * t + phase);

                if (px == 0.0f)
                    path.startNewSubPath (x + px, y);
                else
                    path.lineTo (x + px, y);
            }

            g.strokePath (path, juce::PathStrokeType (1.5f));
        }
    }
};
