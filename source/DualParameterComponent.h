//
// Created by Tyler Teuber on 5/10/25.
//

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Theme.h"

// Base Component Class
class DualParameterComponent : public juce::Component,
                               private juce::AudioProcessorParameter::Listener
{
public:
    DualParameterComponent (juce::RangedAudioParameter* param1,
        juce::RangedAudioParameter* param2)
        : param1 (param1),
          param2 (param2)
    {
        // Register as a listener for both parameters
        param1->addListener (this);
        param2->addListener (this);

        // Set initial values from parameters
        param1Value = param1->getValue();
        param2Value = param2->getValue();
    }

    ~DualParameterComponent() override
    {
        // Remove listeners
        param1->removeListener (this);
        param2->removeListener (this);
    }

    void paint (juce::Graphics& g) override
    {
        // Draw background
        g.fillAll (SECONDARY_COLOR);

        // Calculate dimensions for the drawing
        const auto bounds = getLocalBounds().reduced (20);

        // Draw the main visualization
        g.setColour (TEXT_COLOR);
        drawVisualization (g, bounds);

        // Draw text indicators
        g.setColour (juce::Colours::white);
        g.setFont (14.0f);

        // Display parameter 1 text (bottom left)
        g.drawText (getParam1Text(),
            10,
            getHeight() - 25,
            120,
            20,
            juce::Justification::bottomLeft,
            true);

        // Display parameter 2 text (top right)
        g.drawText (getParam2Text(),
            getWidth() - 120,
            5,
            110,
            20,
            juce::Justification::topRight,
            true);
    }

    void resized() override
    {
        // Ensure square aspect ratio
        const int size = juce::jmin (getWidth(), getHeight());
        setBounds (getX(), getY(), size, size);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        // Store initial mouse position and parameter values for drag
        mouseDownX = e.x;
        mouseDownY = e.y;
        initialParam1Value = param1Value;
        initialParam2Value = param2Value;

        isDragging = true;
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (!isDragging)
            return;

        const auto bounds = getLocalBounds();

        // Calculate vertical movement for parameter 1
        const float verticalDelta = (mouseDownY - e.y) / (bounds.getHeight() - padding);
        const float newParam1Value = juce::jlimit (0.0f, 1.0f, initialParam1Value + verticalDelta);

        // Calculate horizontal movement for parameter 2
        const float horizontalDelta = (e.x - mouseDownX) / (bounds.getWidth() - padding);
        const float newParam2Value = juce::jlimit (0.0f, 1.0f, initialParam2Value + horizontalDelta);

        // Update parameters
        param1->setValueNotifyingHost (newParam1Value);
        param2->setValueNotifyingHost (newParam2Value);
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        isDragging = false;
    }

protected:
    // Parameter pointers
    juce::RangedAudioParameter* param1;
    juce::RangedAudioParameter* param2;

    // Current parameter values
    float param1Value = 0.0f;
    float param2Value = 0.0f;

    float padding = 0.4f; // Padding for drawing

    // Virtual method to be implemented by derived classes
    virtual void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const = 0;

    // Virtual methods for text display that can be overridden
    virtual juce::String getParam1Text() const
    {
        return formatParameterText (param1, param1Value, "{name}: {value}%");
    }

    virtual juce::String getParam2Text() const
    {
        return formatParameterText (param2, param2Value, "{name}: {value}%");
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

        // Start with the template and replace placeholders
        juce::String result = textTemplate;
        result = result.replace ("{name}", name)
                     .replace ("{value}", juce::String (percentValue))
                     .replace ("{fullValue}", juce::String (fullValue, 2));

        return result;
    }

private:
    void parameterValueChanged (int parameterIndex, float newValue) override
    {
        if (parameterIndex == param1->getParameterIndex())
            param1Value = newValue;
        else
            param2Value = newValue;

        repaint();
    }

    void parameterGestureChanged (int, bool) override
    {
        // Not needed for this implementation
    }

    // Mouse drag tracking
    bool isDragging = false;
    int mouseDownX = 0;
    int mouseDownY = 0;
    float initialParam1Value = 0.0f;
    float initialParam2Value = 0.0f;
};