#pragma once

#include "SingleParameterComponent.h"

class TextParameterComponent : public SingleParameterComponent
{
public:
    using SingleParameterComponent::SingleParameterComponent;

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Background
        g.setColour (SECONDARY_COLOR);
        g.fillRoundedRectangle (bounds, 6.0f);

        // Border
        g.setColour (TEXT_COLOR.withAlpha (0.3f));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

        // Name at top
        g.setColour (TEXT_COLOR.withAlpha (0.6f));
        g.setFont (10.0f);
        g.drawText (param->getName (15), bounds.reduced (4.0f, 2.0f),
            juce::Justification::centredTop, true);

        // Value text
        auto valueArea = bounds.reduced (4.0f).withTrimmedTop (14.0f);
        drawValueText (g, valueArea);
    }

    void resized() override
    {
        // Do NOT enforce square aspect ratio
    }

protected:
    virtual void drawValueText (juce::Graphics& g, juce::Rectangle<float> area) const
    {
        g.setColour (TEXT_COLOR);
        g.setFont (13.0f);
        g.drawText (getParameterText(), area, juce::Justification::centred, true);
    }
};
