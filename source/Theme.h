//
// Created by Tyler Teuber on 3/28/25.
//

#pragma once

#include <juce_graphics/juce_graphics.h>

#define PRIMARY_COLOR juce::Colour::fromRGB (15, 20, 20)
#define SECONDARY_COLOR juce::Colour::fromRGB (35, 40, 40)
#define TERTIARY_COLOR juce::Colour::fromRGB (2, 7, 7)
#define BORDER_COLOR juce::Colour::fromRGB (20, 25, 25)
#define TEXT_COLOR juce::Colour::fromRGB (190, 200, 200)
#define TEXT_INACTIVE_COLOR juce::Colour::fromRGB (110, 110, 110)
#define MOD_COLOR juce::Colours::cyan
#define DEBUG_COLOR juce::Colours::red.withAlpha (0.5f)

#define ENV_COLOR juce::Colours::lightgreen
#define LFO_COLOR juce::Colours::gold
#define CONTROLLER_COLOR juce::Colours::lightcoral // aftertouch, expression, mod wheel
#define KEY_VEL_COLOR juce::Colours::deepskyblue // velocity, keyboard

namespace Style
{
    // Font sizes (semantic names)
    constexpr float fontCaption   = 10.0f; // small labels
    constexpr float fontSmall     = 11.0f; // keyboard labels, hover state labels
    constexpr float fontBody      = 12.0f; // toggles, context menus, rate displays
    constexpr float fontLabel     = 13.0f; // tab text, parameter values
    constexpr float fontComponent = 14.0f; // parameter names, filter titles
    constexpr float fontHeading   = 16.0f; // section headers

    // Border radii
    constexpr float radiusSmall  = 4.5f; // tabs, small buttons, thumbs
    constexpr float radiusMedium = 6.0f; // standard controls, displays, tracks
    constexpr float radiusLarge  = 9.0f; // toggles, text parameters

    // Component layout
    constexpr float labelHeight    = 20.0f; // space for label above component box
    constexpr float vizInset       = 20.0f; // inset from box edge to visualization
    constexpr int   componentGap   = 3;     // spacing around components in layout grid

    // Common alpha values
    constexpr float alphaBorder   = 1.0f; // standard border opacity
    constexpr float alphaInactive = 0.5f; // inactive component opacity
    constexpr float alphaMod      = 0.7f; // modulation overlay
    constexpr float alphaModGhost = 0.2f; // bipolar ghost

    // Hover animation
    constexpr float hoverSmoothing = 0.25f; // lerp factor per frame
    constexpr float hoverThreshold = 0.01f; // snap-to-target threshold
    constexpr int   hoverFrameRate = 60;    // timer Hz for hover
}

inline auto getModColor (const juce::String& sourceID) -> juce::Colour
{
    if (sourceID.startsWith ("lfo"))
        return LFO_COLOR;
    if (sourceID.startsWith ("env"))
        return ENV_COLOR;
    if (sourceID == "modWheel" || sourceID == "aftertouch" || sourceID == "expression")
        return CONTROLLER_COLOR;
    return KEY_VEL_COLOR; // keyboard, velocity, fallback
}
