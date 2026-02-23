//
// Created by Tyler Teuber
//

#pragma once

#include "Utility/SingleParameterComponent.h"

class NoiseComponent final : public SingleParameterComponent
{
public:
    explicit NoiseComponent (juce::RangedAudioParameter* noiseParam,
        juce::UndoManager* undoManager = nullptr,
        std::atomic<int>* gestureCount = nullptr)
        : SingleParameterComponent (noiseParam, nullptr, undoManager, gestureCount)
    {
    }

protected:
    void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const override
    {
        const float w = static_cast<float> (bounds.getWidth());
        const float h = static_cast<float> (bounds.getHeight());
        const float cx = static_cast<float> (bounds.getX()) + w * 0.5f;
        const float cy = static_cast<float> (bounds.getY()) + h * 0.5f;
        const float dim = juce::jmin (w, h);

        constexpr int numCircles = 3;
        constexpr int numVertices = 24;
        constexpr float jaggedAmount = 0.20f;

        // Faint radial guide lines
        const float outerRadius = dim * 0.40f;
        g.setOpacity (0.15f);
        for (int i = 0; i < numVertices / 2; ++i)
        {
            const float angle = juce::MathConstants<float>::twoPi * static_cast<float> (i) / static_cast<float> (numVertices / 2);
            g.drawLine (cx, cy,
                cx + std::cos (angle) * outerRadius,
                cy + std::sin (angle) * outerRadius,
                1.0f);
        }

        // Faint concentric guide circles
        for (int c = 0; c < numCircles; ++c)
        {
            const float r = dim * (0.15f + 0.125f * static_cast<float> (c));
            g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 1.0f);
        }
        g.setOpacity (1.0f);

        for (int c = 0; c < numCircles; ++c)
        {
            // Sequential fade-in matching VolumeComponent arc pattern
            const float arcStart = static_cast<float> (c) / static_cast<float> (numCircles);
            const float arcEnd = static_cast<float> (c + 1) / static_cast<float> (numCircles);
            const float opacity = juce::jlimit (0.0f, 1.0f, (paramValue - arcStart) / (arcEnd - arcStart));

            if (opacity <= 0.0f)
                continue;

            g.setOpacity (opacity);

            // Deterministic random per circle so shape is stable across repaints
            juce::Random jaggedRng (42 + c * 1000);

            const float baseRadius = dim * (0.15f + 0.125f * static_cast<float> (c));

            juce::Path circle;
            for (int v = 0; v < numVertices; ++v)
            {
                const float angle = juce::MathConstants<float>::twoPi * static_cast<float> (v) / static_cast<float> (numVertices);
                const float offset = 1.0f + (jaggedRng.nextFloat() * 2.0f - 1.0f) * jaggedAmount;
                const float r = baseRadius * offset;
                const float px = cx + std::cos (angle) * r;
                const float py = cy + std::sin (angle) * r;

                if (v == 0)
                    circle.startNewSubPath (px, py);
                else
                    circle.lineTo (px, py);
            }
            circle.closeSubPath();

            g.strokePath (circle, juce::PathStrokeType (2.0f));
        }

        g.setOpacity (1.0f);
    }
};
