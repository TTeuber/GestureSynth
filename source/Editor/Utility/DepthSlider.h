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
        const float cornerSize = Style::radiusMedium;

        // Track background
        g.setColour (PRIMARY_COLOR);
        g.fillRoundedRectangle (bounds, cornerSize);

        // Fill bar from center
        const float centerX = bounds.getCentreX();
        const float halfWidth = bounds.getWidth() * 0.5f;
        const float fillLeft = value < 0.0f ? centerX + value * halfWidth : centerX;
        const float fillWidth = std::abs (value) * halfWidth;

        if (std::abs (value) > 0.001f)
        {
            g.setColour (TEXT_COLOR.withAlpha (0.45f));
            g.fillRoundedRectangle (fillLeft, bounds.getY(), fillWidth, bounds.getHeight(), cornerSize);
        }

        // Source indicator line
        if (std::abs (value) > 0.001f && sourceValue > 0.001f)
        {
            float modAmount = bipolar
                ? (sourceValue * 2.0f - 1.0f) * value
                : sourceValue * value;
            float lineX = centerX + modAmount * halfWidth;
            lineX = juce::jlimit (bounds.getX(), bounds.getRight(), lineX);

            g.setColour (juce::Colours::white);
            g.drawVerticalLine (static_cast<int> (lineX), bounds.getY() + 1.0f, bounds.getBottom() - 1.0f);
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
            if (onDragStart) onDragStart();
            setValue (0.0f);
            if (onDragEnd) onDragEnd();
            return;
        }
        mouseDownX = e.x;
        initialValue = value;
        isDragging = true;
        if (onDragStart) onDragStart();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (!isDragging)
            return;

        const float sensitivity = e.mods.isShiftDown() ? 600.0f : 150.0f;
        const float delta = (e.x - mouseDownX) / sensitivity;
        setValue (juce::jlimit (-1.0f, 1.0f, initialValue + delta));
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (isDragging && onDragEnd)
            onDragEnd();
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

    void setSourceValue (float v)
    {
        constexpr float alpha = 0.25f;
        float smoothed = sourceValue + alpha * (v - sourceValue);
        if (std::abs (smoothed - sourceValue) < 0.0001f)
            return;
        sourceValue = smoothed;
        repaint();
    }

    void setBipolar (bool b)
    {
        if (bipolar == b)
            return;
        bipolar = b;
        repaint();
    }

    std::function<void()> onValueChange;
    std::function<void()> onDragStart;
    std::function<void()> onDragEnd;

private:
    float value = 0.0f;
    float sourceValue = 0.0f;
    bool bipolar = false;
    bool isDragging = false;
    int mouseDownX = 0;
    float initialValue = 0.0f;
};
