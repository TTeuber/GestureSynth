//
// Oscillator Level Concentric Circles Visualization
//

#pragma once

#include "Utility/SingleParameterComponent.h"

class OscLevelComponent : public SingleParameterComponent
{
public:
    using SingleParameterComponent::SingleParameterComponent;

protected:
    void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const override
    {
        // Map 0–1 parameter to 0–4 circle range
        const float size = paramValue * 4.0f;
        const int fullCircles = static_cast<int> (size);
        const float fraction = size - static_cast<float> (fullCircles);

        const float centreX = bounds.getCentreX();
        const float centreY = bounds.getCentreY();
        const float maxRadius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;

        const float baseAlpha = isActive ? 0.8f : 0.4f;

        // Draw full circles
        for (int i = 1; i <= fullCircles; ++i)
        {
            const float radius = maxRadius * (size - static_cast<float> (i) + 1.0f) / 4.0f;
            g.setColour (TEXT_COLOR.withAlpha (baseAlpha));
            g.drawEllipse (centreX - radius, centreY - radius,
                radius * 2.0f, radius * 2.0f, 1.5f);
        }

        // Draw the fractional (growing-in) circle
        if (fraction > 0.01f && fullCircles < 4)
        {
            const float radius = maxRadius * fraction / 4.0f;
            g.setColour (TEXT_COLOR.withAlpha (baseAlpha * fraction));
            g.drawEllipse (centreX - radius, centreY - radius,
                radius * 2.0f, radius * 2.0f, 1.5f);
        }
    }
};
