//
// Created by Tyler Teuber 5/15/25
//

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../../Theme.h"
#include "ModulationModeState.h"
#include "ModulationContextMenu.h"
#include "UIContext.h"

// Base Component Class for single parameter controls
class SingleParameterComponent : public juce::Component,
                                 private juce::AudioProcessorParameter::Listener,
                                 private juce::Timer
{
public:
    explicit SingleParameterComponent (juce::RangedAudioParameter* param,
        juce::AudioParameterBool* activeParam = nullptr,
        const UIContext& ctx = {},
        const juce::String& paramDestID = {})
        : param (param),
          activeParam (activeParam),
          undoManager (ctx.undoManager),
          gestureCount (ctx.gestureCount),
          modModeState (ctx.modModeState),
          paramDestID (paramDestID)
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
        stopTimer();
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

        g.setFont (14.0f);

        // Draw the parameter name at the top (fades out on hover)
        if (hoverAlpha < 0.99f)
        {
            g.setOpacity (isActive ? (1.0f - hoverAlpha) : 0.5f * (1.0f - hoverAlpha));
            g.drawText (param->getName (15),
                bounds.getX(),
                5,
                bounds.getWidth(),
                20,
                juce::Justification::centredTop,
                true);
        }

        // Draw the parameter value at the top (fades in on hover)
        if (hoverAlpha > 0.01f)
        {
            g.setOpacity (isActive ? hoverAlpha : 0.5f * hoverAlpha);
            g.drawText (getParameterText(),
                bounds.getX(),
                5,
                bounds.getWidth(),
                20,
                juce::Justification::centredTop,
                true);
        }

        g.setOpacity (isActive ? 1.0f : 0.5f);

        // Draw the main visualization (implemented by derived classes)
        drawVisualization (g, bounds.withTrimmedTop (20));

        // Draw modulation overlay
        drawModulationOverlay (g, bounds.withTrimmedTop (20));

        // Draw the active toggle button in the top-left if we have an active parameter
        if (activeParam != nullptr)
        {
            constexpr int toggleSize = 12;
            const juce::Rectangle toggleRect (10, 10, toggleSize, toggleSize);

            g.setOpacity (1.0f); // Ensure the toggle is fully visible even when inactive
            g.setColour (SECONDARY_COLOR);
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

    void mouseEnter (const juce::MouseEvent&) override
    {
        hoverTarget = true;
        startTimerHz (60);
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        hoverTarget = false;
        startTimerHz (60);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        // Modulation context menu on right-click in mod mode
        if (e.mods.isRightButtonDown()
            && modModeState != nullptr && modModeState->isModulationMode()
            && paramDestID.isNotEmpty() && isActive)
        {
            auto sourceID = modModeState->getTargetSourceID();
            if (modModeState->findSlotIndex (sourceID, paramDestID) >= 0)
            {
                showModulationContextMenu (this, modModeState,
                    { { param->getName (15), sourceID, paramDestID } }, e.getScreenPosition());
            }
            return;
        }

        // Check if clicking the toggle button
        if (activeParam != nullptr)
        {
            const juce::Rectangle toggleRect (10, 10, 12, 12);
            if (toggleRect.contains (e.getPosition()) || e.mods.isRightButtonDown())
            {
                if (undoManager != nullptr)
                    undoManager->beginNewTransaction();
                activeParam->beginChangeGesture();
                activeParam->setValueNotifyingHost (isActive ? 0.0f : 1.0f);
                activeParam->endChangeGesture();
                return;
            }
        }

        // Modulation mode handling
        if (modModeState != nullptr && modModeState->isModulationMode())
        {
            if (paramDestID.isEmpty())
                return;

            if (!isActive)
                return;

            auto sourceID = modModeState->getTargetSourceID();
            int slot = modModeState->getOrCreateSlot (sourceID, paramDestID);
            if (slot < 0)
                return;

            if (auto* um = modModeState->getUndoManager())
                um->beginNewTransaction();
            if (gestureCount != nullptr)
                ++(*gestureCount);

            modDragInitialDepth = modModeState->getDepth (sourceID, paramDestID);
            mouseDownY = e.y;
            isModDragging = true;
            return;
        }

        // Only process drag if the component is active
        if (!isActive)
            return;

        // Store initial mouse position and parameter value for drag
        mouseDownY = e.y;
        initialParamValue = paramValue;

        if (undoManager != nullptr)
            undoManager->beginNewTransaction();
        param->beginChangeGesture();
        if (gestureCount != nullptr)
            ++(*gestureCount);

        isDragging = true;
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (isModDragging && modModeState != nullptr)
        {
            auto sourceID = modModeState->getTargetSourceID();
            const auto bounds = getLocalBounds();

            float verticalDelta = (mouseDownY - e.y) / (bounds.getHeight() - padding) / 2.0f;
            float newDepth = juce::jlimit (-1.0f, 1.0f, modDragInitialDepth + verticalDelta);
            modModeState->setDepth (sourceID, paramDestID, newDepth);
            repaint();
            return;
        }

        if (!isDragging || !isActive)
            return;

        const auto bounds = getLocalBounds();

        // Calculate vertical movement for the parameter
        // Moving up increases the value
        const float verticalDelta = (mouseDownY - e.y) / (bounds.getHeight() - padding) / 2.0f;
        const float newParamValue = juce::jlimit (0.0f, 1.0f, initialParamValue + verticalDelta);

        // Update parameter
        param->setValueNotifyingHost (newParamValue);
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (isModDragging)
        {
            if (gestureCount != nullptr)
                --(*gestureCount);
            isModDragging = false;
            return;
        }
        if (isDragging)
        {
            param->endChangeGesture();
            if (gestureCount != nullptr)
                --(*gestureCount);
        }
        isDragging = false;
    }

protected:
    // Parameter pointers
    juce::RangedAudioParameter* param;
    juce::AudioParameterBool* activeParam;

    // Active state
    bool isActive = true;

    // Hover state
    float hoverAlpha = 0.0f;
    bool hoverTarget = false;

    // Current parameter value
    float paramValue = 0.0f;

    float padding = 0.4f; // Padding for drawing

    juce::UndoManager* undoManager = nullptr;
    std::atomic<int>* gestureCount = nullptr;

    // Modulation mode
    ModulationModeState* modModeState = nullptr;
    juce::String paramDestID;

    void rebindParam (juce::RangedAudioParameter* newParam)
    {
        param->removeListener (this);
        param = newParam;
        paramValue = param->getValue();
        param->addListener (this);
        repaint();
    }

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

    void timerCallback() override
    {
        const float target = hoverTarget ? 1.0f : 0.0f;
        hoverAlpha += 0.25f * (target - hoverAlpha);
        if (std::abs (hoverAlpha - target) < 0.01f)
        {
            hoverAlpha = target;
            stopTimer();
        }
        repaint();
    }

    void parameterGestureChanged (int, bool) override
    {
        // Not needed for this implementation
    }

    void drawModulationOverlay (juce::Graphics& g, const juce::Rectangle<int>& bounds)
    {
        if (modModeState == nullptr || !modModeState->isModulationMode())
            return;
        if (paramDestID.isEmpty())
            return;

        auto sourceID = modModeState->getTargetSourceID();
        float depth = modModeState->getDepth (sourceID, paramDestID);

        if (std::abs (depth) < 0.001f)
            return;

        bool bipolar = modModeState->isBipolar (sourceID, paramDestID);

        // Draw a simple indicator line at the modulated position
        float modValue = juce::jlimit (0.0f, 1.0f, paramValue + depth);
        float modY = bounds.getBottom() - modValue * bounds.getHeight();

        g.setColour (getModColor (sourceID).withAlpha (0.7f));
        g.drawHorizontalLine (static_cast<int> (modY), static_cast<float> (bounds.getX()), static_cast<float> (bounds.getRight()));

        if (bipolar)
        {
            float ghostValue = juce::jlimit (0.0f, 1.0f, paramValue - depth);
            float ghostY = bounds.getBottom() - ghostValue * bounds.getHeight();
            g.setColour (getModColor (sourceID).withAlpha (0.2f));
            g.drawHorizontalLine (static_cast<int> (ghostY), static_cast<float> (bounds.getX()), static_cast<float> (bounds.getRight()));
        }
    }

    // Mouse drag tracking
    bool isDragging = false;
    bool isModDragging = false;
    int mouseDownY = 0;
    float initialParamValue = 0.0f;
    float modDragInitialDepth = 0.0f;
};
