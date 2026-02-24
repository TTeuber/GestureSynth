//
// Shared icon drawing functions for modulation controls
//

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Theme.h"

namespace ModulationIcons
{

inline void drawBipolarIcon (juce::Graphics& g, juce::Rectangle<float> bounds, bool bipolar)
{
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    const float hw = bounds.getWidth() * 0.35f;
    const float ah = bounds.getHeight() * 0.15f;

    juce::Path icon;

    if (bipolar)
    {
        // Double-sided arrow <-->
        icon.startNewSubPath (cx - hw, cy);
        icon.lineTo (cx + hw, cy);
        icon.startNewSubPath (cx - hw + ah, cy - ah);
        icon.lineTo (cx - hw, cy);
        icon.lineTo (cx - hw + ah, cy + ah);
        icon.startNewSubPath (cx + hw - ah, cy - ah);
        icon.lineTo (cx + hw, cy);
        icon.lineTo (cx + hw - ah, cy + ah);
    }
    else
    {
        // Single right arrow -->
        icon.startNewSubPath (cx - hw, cy);
        icon.lineTo (cx + hw, cy);
        icon.startNewSubPath (cx + hw - ah, cy - ah);
        icon.lineTo (cx + hw, cy);
        icon.lineTo (cx + hw - ah, cy + ah);
    }

    g.setColour (TEXT_COLOR);
    g.strokePath (icon, juce::PathStrokeType (1.5f));
}

inline void drawBypassIcon (juce::Graphics& g, juce::Rectangle<float> bounds, bool bypassed)
{
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.4f;

    const float alpha = bypassed ? 1.0f : 0.3f;
    g.setColour (TEXT_COLOR.withAlpha (alpha));

    g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 1.5f);

    const float offset = r * 0.707f;
    g.drawLine (cx - offset, cy - offset, cx + offset, cy + offset, 1.5f);
}

inline void drawClearIcon (juce::Graphics& g, juce::Rectangle<float> bounds, bool hover)
{
    const float alpha = hover ? 0.7f : 0.3f;
    g.setColour (TEXT_COLOR.withAlpha (alpha));

    float side = juce::jmin (bounds.getWidth(), bounds.getHeight());
    auto square = bounds.withSizeKeepingCentre (side, side);

    g.drawLine (square.getX(), square.getY(),
                square.getRight(), square.getBottom(), 1.5f);
    g.drawLine (square.getRight(), square.getY(),
                square.getX(), square.getBottom(), 1.5f);
}

} // namespace ModulationIcons
