//
// Created by Tyler Teuber 5/15/25
//

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Theme.h"

// Base Component Class for single parameter controls
class SingleParameterComponent : public juce::Component,
                                 private juce::AudioProcessorParameter::Listener
{
public:
    explicit SingleParameterComponent (juce::RangedAudioParameter* param,
        juce::AudioParameterBool* activeParam = nullptr)
        : param (param),
          activeParam (activeParam)
    {
        // Register as a listener for the parameter
        param->addListener (this);

        // Set initial value from parameter
        paramValue = param->getValue();

        // Register as listener for active parameter if provided
        if (activeParam != nullptr)
        {
            activeParam->addListener (this);
            isActive = activeParam->get();
        }
        else
        {
            isActive = true; // Default to active if no parameter provided
        }
    }

    ~SingleParameterComponent() override
    {
        // Remove listeners
        param->removeListener (this);

        if (activeParam != nullptr)
            activeParam->removeListener (this);
    }

    void paint (juce::Graphics& g) override
    {
        // Draw background
        g.fillAll (SECONDARY_COLOR);

        // Calculate dimensions for the drawing
        const auto bounds = getLocalBounds().reduced (10);

        // Apply darkening if inactive
        if (!isActive)
        {
            g.setOpacity (0.5f);
            g.setColour (TEXT_INACTIVE_COLOR);
        }
        else
        {
            g.setColour (TEXT_COLOR);
        }

        // Draw the parameter name at the top
        g.setFont (14.0f);
        g.drawText (param->getName (15),
            bounds.getX(),
            5,
            bounds.getWidth(),
            20,
            juce::Justification::centredTop,
            true);

        // Draw the main visualization (implemented by derived classes)
        drawVisualization (g, bounds.withTrimmedTop (20).withTrimmedBottom (20));

        // Draw value at the bottom
        g.drawText (getParameterText(),
            bounds.getX(),
            bounds.getBottom() - 20,
            bounds.getWidth(),
            20,
            juce::Justification::centredBottom,
            true);

        // Draw the active toggle button in the top-left if we have an active parameter
        if (activeParam != nullptr)
        {
            constexpr int toggleSize = 12;
            const juce::Rectangle toggleRect (10, 10, toggleSize, toggleSize);

            g.setOpacity (1.0f); // Ensure the toggle is fully visible even when inactive
            g.setColour (juce::Colours::white);
            g.drawRect (toggleRect, 1.0f);

            if (isActive)
            {
                g.fillRect (toggleRect.reduced (2));
            }
        }
    }

    void resized() override
    {
        // Ensure square aspect ratio
        const int size = juce::jmin (getWidth(), getHeight());
        setBounds (getX(), getY(), size, size);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        // Check if clicking the toggle button
        if (activeParam != nullptr)
        {
            const juce::Rectangle toggleRect (10, 10, 12, 12);
            if (toggleRect.contains (e.getPosition()) || e.mods.isRightButtonDown())
            {
                // Toggle the active state
                activeParam->setValueNotifyingHost (isActive ? 0.0f : 1.0f);
                return;
            }
        }

        // Only process drag if the component is active
        if (!isActive)
            return;

        // Store initial mouse position and parameter value for drag
        mouseDownY = e.y;
        initialParamValue = paramValue;

        isDragging = true;
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (!isDragging || !isActive)
            return;

        const auto bounds = getLocalBounds();

        // Calculate vertical movement for the parameter
        // Moving up increases the value
        const float verticalDelta = (mouseDownY - e.y) / (bounds.getHeight() - padding);
        const float newParamValue = juce::jlimit (0.0f, 1.0f, initialParamValue + verticalDelta);

        // Update parameter
        param->setValueNotifyingHost (newParamValue);
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        isDragging = false;
    }

protected:
    // Parameter pointers
    juce::RangedAudioParameter* param;
    juce::AudioParameterBool* activeParam;

    // Active state
    bool isActive = true;

    // Current parameter value
    float paramValue = 0.0f;

    float padding = 0.4f; // Padding for drawing

    // Virtual method to be implemented by derived classes
    virtual void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const {}

    // Virtual method for text display that can be overridden
    virtual juce::String getParameterText() const
    {
        return formatParameterText (param, paramValue, "{value} {unit}");
    }

    // Helper method to format parameter text using templates
    juce::String formatParameterText (juce::RangedAudioParameter* param,
        float normValue,
        const juce::String& textTemplate) const
    {
        // Get the parameter name
        juce::String name = param->getName (15);

        // Calculate percentage value (0-100)
        int percentValue = static_cast<int> (normValue * 100.0f);

        // Get the full value from the normalized value (using parameter mapping)
        float fullValue = param->convertFrom0to1 (normValue);

        // Get display text from the parameter with correct units
        juce::String valueWithUnit = param->getText (normValue, 10);

        // Split value and unit (if possible)
        juce::String value, unit;
        if (valueWithUnit.containsChar (' '))
        {
            value = valueWithUnit.upToLastOccurrenceOf (" ", false, false);
            unit = valueWithUnit.fromLastOccurrenceOf (" ", false, false);
        }
        else
        {
            value = valueWithUnit;
            unit = "";
        }

        // Start with the template and replace placeholders
        juce::String result = textTemplate;
        result = result.replace ("{name}", name)
                     .replace ("{value}", value)
                     .replace ("{unit}", unit)
                     .replace ("{percent}", juce::String (percentValue))
                     .replace ("{fullValue}", juce::String (fullValue, 2));

        return result;
    }

private:
    void parameterValueChanged (int parameterIndex, float newValue) override
    {
        if (parameterIndex == param->getParameterIndex())
            paramValue = newValue;
        else if (activeParam != nullptr && parameterIndex == activeParam->getParameterIndex())
            isActive = activeParam->get();

        repaint();
    }

    void parameterGestureChanged (int, bool) override
    {
        // Not needed for this implementation
    }

    // Mouse drag tracking
    bool isDragging = false;
    int mouseDownY = 0;
    float initialParamValue = 0.0f;
};