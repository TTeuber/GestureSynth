//
// Created by Tyler Teuber on 3/28/25.
//

#pragma once

#include <juce_graphics/juce_graphics.h>

#define PRIMARY_COLOR juce::Colour::fromRGB (20, 25, 25)
#define SECONDARY_COLOR juce::Colour::fromRGB (30, 35, 35)
#define TEXT_COLOR juce::Colour::fromRGB (220, 220, 220)
#define TEXT_INACTIVE_COLOR juce::Colour::fromRGB (110, 110, 110)
#define MOD_COLOR juce::Colours::cyan

inline auto getModColor (const juce::String& sourceID) -> juce::Colour
{
    if (sourceID.startsWith ("lfo"))
        return juce::Colours::yellow;
    if (sourceID.startsWith ("env"))
        return juce::Colours::green;
    if (sourceID == "modWheel" || sourceID == "aftertouch" || sourceID == "expression")
        return juce::Colours::magenta;
    return juce::Colours::cyan; // keyboard, velocity, fallback
}
