//
// Created by Tyler Teuber
//

#pragma once

#include "Utility/SingleParameterComponent.h"

class VolumeComponent final : public SingleParameterComponent
{
public:
    explicit VolumeComponent (juce::RangedAudioParameter* volumeParam,
        juce::UndoManager* undoManager = nullptr,
        std::atomic<int>* gestureCount = nullptr)
        : SingleParameterComponent (volumeParam, nullptr, undoManager, gestureCount)
    {
    }

protected:
    void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const override
    {
        const float w = static_cast<float> (bounds.getWidth());
        const float h = static_cast<float> (bounds.getHeight());
        const float x = static_cast<float> (bounds.getX());
        const float y = static_cast<float> (bounds.getY());

        // Speaker dimensions
        const float driverW = w * 0.08f;
        const float driverH = h * 0.25f;
        const float coneW = w * 0.15f;
        const float coneH = h * 0.45f;

        // Position the speaker on the left side of center
        const float speakerStartX = x + w * 0.2f;
        const float centreY = y + h * 0.5f;

        // Draw the speaker as a single combined shape (driver + cone)
        const float coneLeft = speakerStartX + driverW;
        const float coneRight = coneLeft + coneW;

        juce::Path speaker;
        speaker.startNewSubPath (speakerStartX, centreY - driverH * 0.5f);
        speaker.lineTo (coneLeft, centreY - driverH * 0.5f);
        speaker.lineTo (coneRight, centreY - coneH * 0.5f);
        speaker.lineTo (coneRight, centreY + coneH * 0.5f);
        speaker.lineTo (coneLeft, centreY + driverH * 0.5f);
        speaker.lineTo (speakerStartX, centreY + driverH * 0.5f);
        speaker.closeSubPath();
        g.strokePath (speaker, juce::PathStrokeType (2.0f));

        // Draw sound wave arcs with fade-in opacity
        const float arcStartX = coneRight + w * 0.04f;
        const float arcSpacing = w * 0.09f;
        const float halfArc = juce::MathConstants<float>::pi * 0.25f; // 45°
        const float centreAngle = juce::MathConstants<float>::pi * 0.5f; // 3 o'clock
        constexpr int numArcs = 3;

        for (int i = 0; i < numArcs; ++i)
        {
            const float arcStart = static_cast<float> (i) / static_cast<float> (numArcs);
            const float arcEnd = static_cast<float> (i + 1) / static_cast<float> (numArcs);
            const float opacity = juce::jlimit (0.0f, 1.0f, (paramValue - arcStart) / (arcEnd - arcStart));

            if (opacity <= 0.0f)
                continue;

            g.setOpacity (opacity);
            const float radius = (static_cast<float> (i) + 1.0f) * arcSpacing;
            juce::Path arc;
            arc.addCentredArc (arcStartX,
                centreY,
                radius,
                radius,
                0.0f,
                centreAngle - halfArc,
                centreAngle + halfArc,
                true);
            g.strokePath (arc, juce::PathStrokeType (3.0f));
        }
        g.setOpacity (1.0f);
    }
};
