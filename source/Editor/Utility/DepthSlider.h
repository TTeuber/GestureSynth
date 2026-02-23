#pragma once

#include "../../Theme.h"
#include <juce_gui_basics/juce_gui_basics.h>

class DepthSlider : public juce::Component
{
public:
    DepthSlider() { setRepaintsOnMouseActivity (true); }

    void paint (juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat().reduced (2.0f, 4.0f);
        const float cornerSize = 4.0f;

        // Track background
        g.setColour (PRIMARY_COLOR);
        g.fillRoundedRectangle (bounds, cornerSize);

        // Fill bar from center
        const float centerX = bounds.getCentreX();
        const float fillLeft = value < 0.0f ? centerX + value * (bounds.getWidth() * 0.5f) : centerX;
        const float fillWidth = std::abs (value) * (bounds.getWidth() * 0.5f);

        if (std::abs (value) > 0.001f)
        {
            g.setColour (TEXT_COLOR.withAlpha (0.45f));
            g.fillRoundedRectangle (fillLeft, bounds.getY(), fillWidth, bounds.getHeight(), cornerSize);
        }

        // Center line marker
        g.setColour (TEXT_COLOR.withAlpha (0.3f));
        g.drawVerticalLine (static_cast<int> (centerX), bounds.getY() + 2.0f, bounds.getBottom() - 2.0f);

        // Border
        g.setColour (TEXT_COLOR.withAlpha (isMouseOver() ? 0.4f : 0.2f));
        g.drawRoundedRectangle (bounds, cornerSize, 1.0f);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.getNumberOfClicks() >= 2)
        {
            setValue (0.0f);
            return;
        }
        mouseDownY = e.y;
        initialValue = value;
        isDragging = true;
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (!isDragging)
            return;

        const float sensitivity = e.mods.isShiftDown() ? 600.0f : 150.0f;
        const float delta = (mouseDownY - e.y) / sensitivity;
        setValue (juce::jlimit (-1.0f, 1.0f, initialValue + delta));
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        isDragging = false;
    }

    void mouseDoubleClick (const juce::MouseEvent&) override
    {
        setValue (0.0f);
    }

    float getValue() const { return value; }

    void setValue (float newValue)
    {
        newValue = juce::jlimit (-1.0f, 1.0f, newValue);
        if (std::abs (newValue - value) < 0.0001f)
            return;
        value = newValue;
        repaint();
        if (onValueChange)
            onValueChange();
    }

    std::function<void()> onValueChange;

private:
    float value = 0.0f;
    bool isDragging = false;
    int mouseDownY = 0;
    float initialValue = 0.0f;
};
