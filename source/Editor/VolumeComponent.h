//
// Created by Tyler Teuber
//

#pragma once

#include "Utility/SingleParameterComponent.h"

class VolumeComponent final : public SingleParameterComponent
{
public:
    explicit VolumeComponent (juce::RangedAudioParameter* volumeParam,
        const UIContext& ctx = {},
        const juce::String& paramDestIDToUse = {})
        : SingleParameterComponent (volumeParam, nullptr, ctx, paramDestIDToUse)
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
        const float speakerW = driverW + coneW;

        // The waves span from the cone out to (gap + outermost arc radius), which
        // both scale with the value. Centre the whole picture (speaker + waves)
        // so the speaker slides left exactly as far as the waves push right.
        const float arcGap = w * 0.04f;
        const float maxArcRadius = juce::jmin (w * 0.6f, h * 0.6f);
        const float waveExtent = (arcGap + maxArcRadius) * paramValue;
        const float speakerStartX = x + (w - (speakerW + waveExtent)) * 0.5f;
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

        // Sound wave arcs radiate outward from the cone as volume increases.
        // Arc i exists when size >= i; the outermost arc continuously grows
        // and new arcs are born at the cone (same emerge-and-grow pattern
        // as the concentric-circle components).
        const float arcStartX = coneRight + arcGap * paramValue;
        const float halfArc = juce::MathConstants<float>::pi * 0.25f; // 45°
        const float centreAngle = juce::MathConstants<float>::pi * 0.5f; // 3 o'clock
        constexpr float numArcs = 3.0f;

        const float size = paramValue * numArcs;
        const int fullArcs = static_cast<int> (size);
        const float fraction = size - static_cast<float> (fullArcs);

        auto drawArc = [&] (float radius)
        {
            juce::Path arc;
            arc.addCentredArc (arcStartX,
                centreY,
                radius,
                radius,
                0.0f,
                centreAngle - halfArc,
                centreAngle + halfArc,
                true);
            g.strokePath (arc, juce::PathStrokeType (2.0f));
        };

        // Draw full arcs
        for (int i = 1; i <= fullArcs; ++i)
            drawArc (maxArcRadius * (size - static_cast<float> (i) + 1.0f) / numArcs);

        // Draw the fractional (growing-in) arc — appears from the cone
        if (fraction > 0.01f && fullArcs < static_cast<int> (numArcs))
        {
            g.setOpacity (fraction);
            drawArc (maxArcRadius * fraction / numArcs);
            g.setOpacity (1.0f);
        }
    }
};
