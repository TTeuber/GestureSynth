#pragma once

#include "../../Theme.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace ConnectorPainting
{
    // Draws a horizontal connector bridging a smaller source component (on the left)
    // to a taller target component (on the right), with a fillet at the height mismatch
    // and a pill-shaped hole through the bridge.
    inline void drawHorizontalConnector (juce::Graphics& g,
                                          juce::Rectangle<int> source,
                                          juce::Rectangle<int> target)
    {
        constexpr float radius = Style::radiusMedium;
        constexpr float borderAlpha = Style::alphaBorder;
        constexpr float borderWidth = 1.0f;
        constexpr int gap = Style::componentGap * 2; // total gap between components
        constexpr float filletRadius = radius + static_cast<float> (gap);
        constexpr int pillPadding = 10;

        // Block connector: bridge the gap, overlapping into both components
        // to cover their rounded corners
        const int blockX = source.getRight() - static_cast<int> (radius);
        const int blockY = source.getY();
        const int blockW = (target.getX() - source.getRight()) + static_cast<int> (radius) * 2;
        const int blockH = source.getHeight();

        // Fill block connector
        g.setColour (SECONDARY_COLOR);
        g.fillRect (blockX, blockY, blockW, blockH);

        // Top border
        g.setColour (BORDER_COLOR.withAlpha (borderAlpha));
        g.fillRect (static_cast<float> (source.getRight() - radius),
                    static_cast<float> (blockY) - 0.0f,
                    static_cast<float> (target.getX() - source.getRight()) + radius * 2.0f,
                    borderWidth);

        // Fillet corner: concave curve where source ends but target continues down
        const float cornerX = static_cast<float> (target.getX());
        const float cornerY = static_cast<float> (source.getBottom());

        // Fill path to carve out the concave fillet (mirrored: small component is on left)
        juce::Path fillPath;
        fillPath.startNewSubPath (cornerX, cornerY);
        fillPath.lineTo (cornerX - filletRadius, cornerY);
        fillPath.quadraticTo (cornerX, cornerY,
                              cornerX, cornerY + filletRadius);
        fillPath.lineTo (cornerX + 1, cornerY + filletRadius);
        fillPath.lineTo (cornerX + 1, cornerY - 1);
        fillPath.lineTo (cornerX - filletRadius, cornerY - 1);
        fillPath.closeSubPath();
        g.setColour (SECONDARY_COLOR);
        g.fillPath (fillPath);

        // Fillet border arc (curves from left of corner down to below corner)
        juce::Path borderPath;
        borderPath.startNewSubPath (cornerX - filletRadius - 1, cornerY - 0.5f);
        borderPath.lineTo (cornerX - filletRadius + 0.5f, cornerY - 0.5f);
        borderPath.quadraticTo (cornerX, cornerY,
                                cornerX + 0.5f, cornerY + filletRadius - 0.5f);
        borderPath.lineTo (cornerX + 0.5f, cornerY + filletRadius);
        g.setColour (BORDER_COLOR.withAlpha (borderAlpha));
        g.strokePath (borderPath, juce::PathStrokeType (borderWidth));

        // Pill-shaped hole
        const float pillX = static_cast<float> (source.getRight());
        const float pillY = static_cast<float> (source.getY() + pillPadding);
        const float pillW = static_cast<float> (gap);
        const float pillH = static_cast<float> (source.getHeight() - pillPadding * 2);
        const float pillRadius = pillW / 2.0f;

        g.setColour (PRIMARY_COLOR);
        g.fillRoundedRectangle (pillX, pillY, pillW, pillH, pillRadius);

        g.setColour (BORDER_COLOR.withAlpha (borderAlpha));
        g.drawRoundedRectangle (pillX, pillY, pillW, pillH, pillRadius, borderWidth);
    }
}
