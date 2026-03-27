#pragma once

#include "SingleParameterComponent.h"
#include "PaintHelpers.h"

class TextParameterComponent : public SingleParameterComponent
{
public:
    using SingleParameterComponent::SingleParameterComponent;

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        PaintHelpers::drawComponentBox (g, bounds, Style::radiusLarge);

        // Name at top
        g.setColour (TEXT_COLOR.withAlpha (0.6f));
        g.setFont (Style::fontCaption);
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
        g.setFont (Style::fontLabel);
        g.drawText (getParameterText(), area, juce::Justification::centred, true);
    }
};
