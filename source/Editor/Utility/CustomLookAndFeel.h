#pragma once

#include "../../Theme.h"
#include <juce_gui_basics/juce_gui_basics.h>

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel()
    {
        // ComboBox colours — applied automatically to all ComboBoxes using this L&F
        setColour (juce::ComboBox::backgroundColourId, SECONDARY_COLOR);
        setColour (juce::ComboBox::textColourId, TEXT_COLOR);
        setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
        setColour (juce::ComboBox::arrowColourId, TEXT_COLOR);

        // PopupMenu colours for consistent dropdown appearance
        setColour (juce::PopupMenu::backgroundColourId, SECONDARY_COLOR);
        setColour (juce::PopupMenu::textColourId, TEXT_COLOR);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, SECONDARY_COLOR.brighter (0.15f));
        setColour (juce::PopupMenu::highlightedTextColourId, TEXT_COLOR);
    }

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
        auto area = button.getActiveArea().toFloat().reduced ((float) Style::componentGap, (float) Style::componentGap);

        // Background
        auto bgColour = SECONDARY_COLOR;
        if (isMouseOver && ! button.isFrontTab())
            bgColour = bgColour.brighter (0.06f);
        g.setColour (bgColour);
        g.fillRoundedRectangle (area, Style::radiusSmall);

        // Selected border
        if (button.isFrontTab())
        {
            g.setColour (TEXT_COLOR);
            g.drawRoundedRectangle (area.reduced (0.5f), Style::radiusSmall, 1.5f);
        }

        // Text
        g.setColour (TEXT_COLOR);
        g.setFont (Style::fontLabel);
        g.drawText (button.getButtonText(), area, juce::Justification::centred, false);
    }

    void drawTabButtonText (juce::TabBarButton&, juce::Graphics&,
                            bool, bool) override
    {
        // Handled in drawTabButton
    }

    int getTabButtonBestWidth (juce::TabBarButton& button, int /*tabDepth*/) override
    {
        // Match the grid column width used by tab content (availableWidth / 6)
        return button.getTabbedButtonBar().getWidth() / 6;
    }
};
