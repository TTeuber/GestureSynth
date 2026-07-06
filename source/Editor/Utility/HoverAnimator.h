#pragma once

#include "../../Theme.h"
#include <juce_gui_basics/juce_gui_basics.h>

class HoverAnimator : private juce::Timer
{
public:
    explicit HoverAnimator (juce::Component& ownerToUse) : owner (ownerToUse) {}

    void setHovered (bool hovered)
    {
        hoverTarget = hovered;
        startTimerHz (Style::hoverFrameRate);
    }

    float getAlpha() const { return hoverAlpha; }

private:
    void timerCallback() override
    {
        const float target = hoverTarget ? 1.0f : 0.0f;
        hoverAlpha += Style::hoverSmoothing * (target - hoverAlpha);

        if (std::abs (hoverAlpha - target) < Style::hoverThreshold)
        {
            hoverAlpha = target;
            stopTimer();
        }

        owner.repaint();
    }

    juce::Component& owner;
    float hoverAlpha = 0.0f;
    bool hoverTarget = false;
};
