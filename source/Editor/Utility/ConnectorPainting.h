#pragma once

#include "../../Theme.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace ConnectorPainting
{
    // Draws a horizontal connector bridging a smaller source component (on the left)
    // to a taller target component (on the right), with a fillet at the height mismatch
    // and a pill-shaped hole through the bridge.
    inline void chorusComponentConnector (juce::Graphics& g,
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

    // Draws a vertical connector bridging a narrower source component (above)
    // to a wider target component (below), with a fillet at the width mismatch
    // (top-right corner, curving up and to the right).
    inline void oscComponentConnector (juce::Graphics& g,
                                        juce::Rectangle<int> source,
                                        juce::Rectangle<int> target)
    {
        constexpr float radius = Style::radiusMedium;
        constexpr float borderAlpha = Style::alphaBorder;
        constexpr float borderWidth = 1.0f;
        constexpr int gap = Style::componentGap * 2;
        constexpr float filletRadius = radius + static_cast<float> (gap);
        constexpr int pillPadding = 10;

        // Block connector: bridge the gap vertically, overlapping into both components
        const int blockX = source.getX();
        const int blockY = source.getBottom() - static_cast<int> (radius);
        const int blockW = source.getWidth();
        const int blockH = (target.getY() - source.getBottom()) + static_cast<int> (radius) * 2;

        // Fill block connector
        g.setColour (SECONDARY_COLOR);
        g.fillRect (blockX, blockY, blockW, blockH);

        // Left border
        g.setColour (BORDER_COLOR.withAlpha (borderAlpha));
        g.fillRect (static_cast<float> (blockX),
                    static_cast<float> (source.getBottom() - radius),
                    borderWidth,
                    static_cast<float> (target.getY() - source.getBottom()) + radius * 2.0f);

        // Fillet corner: concave curve where source ends but target continues right
        // Corner is at (source.getRight(), target.getY())
        const float cornerX = static_cast<float> (source.getRight());
        const float cornerY = static_cast<float> (target.getY());

        // Fill path for the fillet (curves from above corner to right of corner)
        juce::Path fillPath;
        fillPath.startNewSubPath (cornerX, cornerY);
        fillPath.lineTo (cornerX, cornerY - filletRadius);
        fillPath.quadraticTo (cornerX, cornerY,
                              cornerX + filletRadius, cornerY);
        fillPath.lineTo (cornerX + filletRadius, cornerY + 1);
        fillPath.lineTo (cornerX - 1, cornerY + 1);
        fillPath.lineTo (cornerX - 1, cornerY - filletRadius);
        fillPath.closeSubPath();
        g.setColour (SECONDARY_COLOR);
        g.fillPath (fillPath);

        // Fillet border arc (curves from above corner to right of corner)
        juce::Path borderPath;
        borderPath.startNewSubPath (cornerX - 0.5f, cornerY - filletRadius - 1);
        borderPath.lineTo (cornerX - 0.5f, cornerY - filletRadius + 0.5f);
        borderPath.quadraticTo (cornerX, cornerY,
                                cornerX + filletRadius - 0.5f, cornerY + 0.5f);
        borderPath.lineTo (cornerX + filletRadius, cornerY + 0.5f);
        g.setColour (BORDER_COLOR.withAlpha (borderAlpha));
        g.strokePath (borderPath, juce::PathStrokeType (borderWidth));

        // Pill-shaped hole (horizontal orientation for vertical connector)
        const float pillX = static_cast<float> (source.getX() + pillPadding);
        const float pillY = static_cast<float> (source.getBottom());
        const float pillW = static_cast<float> (source.getWidth() - pillPadding * 2);
        const float pillH = static_cast<float> (gap);
        const float pillRadius = pillH / 2.0f;

        g.setColour (PRIMARY_COLOR);
        g.fillRoundedRectangle (pillX, pillY, pillW, pillH, pillRadius);

        g.setColour (BORDER_COLOR.withAlpha (borderAlpha));
        g.drawRoundedRectangle (pillX, pillY, pillW, pillH, pillRadius, borderWidth);
    }

    // Draws a horizontal connector between two equal-height components
    // with top and bottom borders and two stacked pill holes.
    inline void drawHorizontalConnectorSimple (juce::Graphics& g,
                                                juce::Rectangle<int> source,
                                                juce::Rectangle<int> target)
    {
        constexpr float radius = Style::radiusMedium;
        constexpr float borderAlpha = Style::alphaBorder;
        constexpr float borderWidth = 1.0f;
        constexpr int gap = Style::componentGap * 2;
        // Block connector
        const int blockX = source.getRight() - static_cast<int> (radius);
        const int blockY = source.getY();
        const int blockW = (target.getX() - source.getRight()) + static_cast<int> (radius) * 2;
        const int blockH = source.getHeight();

        g.setColour (SECONDARY_COLOR);
        g.fillRect (blockX, blockY, blockW, blockH);

        // Top border
        g.setColour (BORDER_COLOR.withAlpha (borderAlpha));
        g.fillRect (static_cast<float> (source.getRight() - radius),
                    static_cast<float> (blockY),
                    static_cast<float> (target.getX() - source.getRight()) + radius * 2.0f,
                    borderWidth);

        // Bottom border
        g.fillRect (static_cast<float> (source.getRight() - radius),
                    static_cast<float> (source.getBottom()) - borderWidth,
                    static_cast<float> (target.getX() - source.getRight()) + radius * 2.0f,
                    borderWidth);

        // Pill hole aligned with inner box of DualParameterComponent
        // Inner box: outerBounds.reduced(4), minus top/bottom labels (20px each), reduced(4)
        constexpr int innerBoxInset = 4 + static_cast<int> (Style::labelHeight) + 4; // 28px
        const float pillX = static_cast<float> (source.getRight());
        const float pillY = static_cast<float> (source.getY() + innerBoxInset);
        const float pillW = static_cast<float> (gap);
        const float pillH = static_cast<float> (source.getHeight() - innerBoxInset * 2);
        const float pillRadius = pillW / 2.0f;

        g.setColour (PRIMARY_COLOR);
        g.fillRoundedRectangle (pillX, pillY, pillW, pillH, pillRadius);

        g.setColour (BORDER_COLOR.withAlpha (borderAlpha));
        g.drawRoundedRectangle (pillX, pillY, pillW, pillH, pillRadius, borderWidth);
    }

    // Draws a "window mullion" connector across a 2x2 grid of equal-size components:
    // four bridges (two horizontal across rows, two vertical down columns) each with
    // its own pill hole. No corner fillets — the bridges occupy disjoint regions.
    inline void drawGridConnector (juce::Graphics& g,
                                   juce::Rectangle<int> topLeft,
                                   juce::Rectangle<int> topRight,
                                   juce::Rectangle<int> bottomLeft,
                                   juce::Rectangle<int> bottomRight)
    {
        constexpr float radius = Style::radiusMedium;
        constexpr float borderAlpha = Style::alphaBorder;
        constexpr float borderWidth = 1.0f;
        constexpr int gap = Style::componentGap * 2;
        constexpr int pillPadding = 10;

        auto drawHorizontalBridge = [&] (juce::Rectangle<int> left, juce::Rectangle<int> right, bool isTop)
        {
            const int blockH = left.getHeight() / 2;
            const int blockX = left.getRight() - static_cast<int> (radius);
            int blockY = left.getY();
            if (!isTop) blockY += blockH;
            const int blockW = (right.getX() - left.getRight()) + static_cast<int> (radius) * 2;

            g.setColour (SECONDARY_COLOR);
            g.fillRect (blockX, blockY, blockW, blockH);

            g.setColour (BORDER_COLOR.withAlpha (borderAlpha));
            if (isTop)
            {
                g.fillRect (static_cast<float> (blockX),
                            static_cast<float> (blockY),
                            static_cast<float> (blockW),
                            borderWidth);
            }
            else
            {
                g.fillRect (static_cast<float> (blockX),
                            static_cast<float> (left.getBottom()) - borderWidth,
                            static_cast<float> (blockW),
                            borderWidth);
            }

            const float pillX = static_cast<float> (left.getRight());
            const float pillY = static_cast<float> (left.getY() + pillPadding);
            const float pillW = static_cast<float> (gap);
            const float pillH = static_cast<float> (left.getHeight() - pillPadding * 2);
            const float pillRadius = pillW / 2.0f;

            g.setColour (PRIMARY_COLOR);
            g.fillRoundedRectangle (pillX, pillY, pillW, pillH, pillRadius);

            g.setColour (BORDER_COLOR.withAlpha (borderAlpha));
            g.drawRoundedRectangle (pillX, pillY, pillW, pillH, pillRadius, borderWidth);
        };

        auto drawVerticalBridge = [&] (juce::Rectangle<int> top, juce::Rectangle<int> bottom, bool isLeft)
        {
            const int blockW = top.getWidth() / 2;
            int blockX = isLeft ? top.getX() + 1 : top.getX();
            if (!isLeft) blockX += blockW;
            const int blockY = top.getBottom() - static_cast<int> (radius);
            const int blockH = (bottom.getY() - top.getBottom()) + static_cast<int> (radius) * 2;

            g.setColour (SECONDARY_COLOR);
            g.fillRect (blockX, blockY, blockW, blockH);

            // g.setColour (BORDER_COLOR.withAlpha (borderAlpha));
            // g.fillRect (static_cast<float> (blockX),
            //             static_cast<float> (blockY),
            //             borderWidth,
            //             static_cast<float> (blockH));
            // g.fillRect (static_cast<float> (top.getRight()) - borderWidth,
            //             static_cast<float> (blockY),
            //             borderWidth,
            //             static_cast<float> (blockH));

            const float pillX = static_cast<float> (top.getX() + pillPadding);
            const float pillY = static_cast<float> (top.getBottom());
            const float pillW = static_cast<float> (top.getWidth() - pillPadding * 2);
            const float pillH = static_cast<float> (gap);
            const float pillRadius = pillH / 2.0f;

            g.setColour (PRIMARY_COLOR);
            g.fillRoundedRectangle (pillX, pillY, pillW, pillH, pillRadius);

            g.setColour (BORDER_COLOR.withAlpha (borderAlpha));
            g.drawRoundedRectangle (pillX, pillY, pillW, pillH, pillRadius, borderWidth);
        };

        drawHorizontalBridge (topLeft, topRight, true);
        drawHorizontalBridge (bottomLeft, bottomRight, false);
        drawVerticalBridge (topLeft, bottomLeft, true);
        drawVerticalBridge (topRight, bottomRight, false);
    }

    // Draws a unified background shell for a delay or reverb effect chain:
    // one continuous SECONDARY rounded rectangle with a 1px BORDER stroke
    // spanning all six sub-components, plus six pill-shaped holes at the
    // gaps where connectors used to live. Chain layout:
    //     [  big  ][  mod  ][gridTL|gridTR]
    //                       [gridBL|gridBR]
    inline void drawEffectChainShell (juce::Graphics& g,
                                      juce::Rectangle<int> big,
                                      juce::Rectangle<int> mod,
                                      juce::Rectangle<int> gridTL,
                                      juce::Rectangle<int> gridTR,
                                      juce::Rectangle<int> gridBL,
                                      juce::Rectangle<int> gridBR)
    {
        constexpr float cornerRadius = Style::radiusMedium;
        constexpr float borderAlpha = Style::alphaBorder;
        constexpr float borderWidth = 1.0f;
        constexpr int gap = Style::componentGap * 2;
        constexpr int innerBoxInset = 4 + static_cast<int> (Style::labelHeight) + 4; // 28px, matches DualParameterComponent inner box
        constexpr int gridPillPadding = 10;

        // 1. Unified container: SECONDARY fill + 1px BORDER stroke
        auto shellBounds = big.getUnion (mod)
                              .getUnion (gridTL).getUnion (gridTR)
                              .getUnion (gridBL).getUnion (gridBR)
                              .toFloat();

        g.setColour (SECONDARY_COLOR);
        g.fillRoundedRectangle (shellBounds, cornerRadius);
        g.setColour (BORDER_COLOR.withAlpha (borderAlpha));
        g.drawRoundedRectangle (shellBounds.reduced (0.5f), cornerRadius, borderWidth);

        // 2. Pill holes punched through the shell. PRIMARY fill + BORDER stroke.
        auto drawVerticalPill = [&] (float x, float y, float w, float h)
        {
            const float r = w * 0.5f;
            g.setColour (PRIMARY_COLOR);
            g.fillRoundedRectangle (x, y, w, h, r);
            g.setColour (BORDER_COLOR.withAlpha (borderAlpha));
            g.drawRoundedRectangle (x, y, w, h, r, borderWidth);
        };

        auto drawHorizontalPill = [&] (float x, float y, float w, float h)
        {
            const float r = h * 0.5f;
            g.setColour (PRIMARY_COLOR);
            g.fillRoundedRectangle (x, y, w, h, r);
            g.setColour (BORDER_COLOR.withAlpha (borderAlpha));
            g.drawRoundedRectangle (x, y, w, h, r, borderWidth);
        };

        // big <-> mod (tall vertical pill, aligned with big's inner box)
        drawVerticalPill (static_cast<float> (big.getRight()),
                          static_cast<float> (big.getY() + innerBoxInset),
                          static_cast<float> (gap),
                          static_cast<float> (big.getHeight() - innerBoxInset * 2));

        // mod <-> grid (tall vertical pill, aligned with mod's inner box)
        drawVerticalPill (static_cast<float> (mod.getRight()),
                          static_cast<float> (mod.getY() + innerBoxInset),
                          static_cast<float> (gap),
                          static_cast<float> (mod.getHeight() - innerBoxInset * 2));

        // grid left column <-> right column (single tall vertical pill spanning
        // both rows, matching the innerBoxInset used by big<->mod / mod<->grid)
        drawVerticalPill (static_cast<float> (gridTL.getRight()),
                          static_cast<float> (gridTL.getY() + innerBoxInset),
                          static_cast<float> (gap),
                          static_cast<float> ((gridBL.getBottom() - gridTL.getY()) - innerBoxInset * 2));

        // grid top-left <-> bottom-left (horizontal pill between rows, left column)
        drawHorizontalPill (static_cast<float> (gridTL.getX() + gridPillPadding),
                            static_cast<float> (gridTL.getBottom()),
                            static_cast<float> (gridTL.getWidth() - gridPillPadding * 2),
                            static_cast<float> (gap));

        // grid top-right <-> bottom-right (horizontal pill between rows, right column)
        drawHorizontalPill (static_cast<float> (gridTR.getX() + gridPillPadding),
                            static_cast<float> (gridTR.getBottom()),
                            static_cast<float> (gridTR.getWidth() - gridPillPadding * 2),
                            static_cast<float> (gap));
    }
}
