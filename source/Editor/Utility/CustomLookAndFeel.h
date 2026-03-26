#pragma once

#include "../../Theme.h"
#include <juce_gui_basics/juce_gui_basics.h>

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawTabbedButtonBarBackground (juce::TabbedButtonBar&, juce::Graphics&) override
    {
        // No-op — parent component already paints PRIMARY_COLOR
    }

    void drawTabAreaBehindFrontButton (juce::TabbedButtonBar&, juce::Graphics&, int, int) override
    {
        // No-op — remove default shadow/gradient
    }

    void drawTabButton (juce::TabBarButton& button, juce::Graphics& g,
                        bool isMouseOver, bool /*isMouseDown*/) override
    {
        auto area = button.getActiveArea().toFloat().reduced (2.0f, 4.0f);

        // Background
        auto bgColour = SECONDARY_COLOR;
        if (isMouseOver && ! button.isFrontTab())
            bgColour = bgColour.brighter (0.06f);
        g.setColour (bgColour);
        g.fillRoundedRectangle (area, 3.0f);

        // Selected border
        if (button.isFrontTab())
        {
            g.setColour (TEXT_COLOR);
            g.drawRoundedRectangle (area.reduced (0.5f), 3.0f, 1.5f);
        }

        // Text
        g.setColour (TEXT_COLOR);
        g.setFont (13.0f);
        g.drawText (button.getButtonText(), area, juce::Justification::centred, false);
    }

    void drawTabButtonText (juce::TabBarButton&, juce::Graphics&,
                            bool, bool) override
    {
        // Handled in drawTabButton
    }

    int getTabButtonBestWidth (juce::TabBarButton& button, int /*tabDepth*/) override
    {
        return juce::jmax (80, button.getTabbedButtonBar().getWidth()
                                   / button.getTabbedButtonBar().getNumTabs());
    }
};
