#pragma once

#include "../../Theme.h"
#include <juce_graphics/juce_graphics.h>

namespace PaintHelpers
{
    // Standard component box: SECONDARY_COLOR fill + TEXT_COLOR border
    inline void drawComponentBox (juce::Graphics& g, juce::Rectangle<float> bounds,
                                   float cornerRadius = Style::radiusMedium,
                                   float borderAlpha = Style::alphaBorder)
    {
        g.setColour (SECONDARY_COLOR);
        g.fillRoundedRectangle (bounds, cornerRadius);
        g.setColour (TEXT_COLOR.withAlpha (borderAlpha));
        g.drawRoundedRectangle (bounds.reduced (0.5f), cornerRadius, 1.0f);
    }

    // Hover-button variant: brighter on hover, no border
    inline void drawHoverBox (juce::Graphics& g, juce::Rectangle<float> bounds,
                               bool isMouseOver, float cornerRadius = Style::radiusMedium)
    {
        auto bg = isMouseOver ? SECONDARY_COLOR.brighter (0.15f) : SECONDARY_COLOR;
        g.setColour (bg);
        g.fillRoundedRectangle (bounds, cornerRadius);
    }
}
