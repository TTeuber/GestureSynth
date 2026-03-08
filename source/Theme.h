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

#define ENV_COLOR juce::Colours::lightgreen
#define LFO_COLOR juce::Colours::gold
#define CONTROLLER_COLOR juce::Colours::lightcoral // aftertouch, expression, mod wheel
#define KEY_VEL_COLOR juce::Colours::deepskyblue // velocity, keyboard

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
