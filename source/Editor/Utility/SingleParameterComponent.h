//
// Created by Tyler Teuber 5/15/25
//

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../../Theme.h"
#include "ModulationModeState.h"
#include "ModulationContextMenu.h"
#include "HoverAnimator.h"
#include "InlineParameterEditor.h"
#include "PaintHelpers.h"
#include "UIContext.h"

// Base Component Class for single parameter controls
class SingleParameterComponent : public juce::Component,
                                 private juce::AudioProcessorParameter::Listener
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
        // Remove listeners
        param->removeListener (this);

        if (activeParam != nullptr)
            activeParam->removeListener (this);
    }

    void paint (juce::Graphics& g) override
    {
        // Fill with parent background
        g.fillAll (PRIMARY_COLOR);

        // Outer box
        auto outerBounds = getLocalBounds();
        PaintHelpers::drawComponentBox (g, outerBounds.toFloat());

        auto topLabelArea = getTopLabelBounds();
        auto innerBoxBounds = getInnerBoxBounds();

        // Inner box (extends to bottom)
        PaintHelpers::drawInnerBox (g, innerBoxBounds.toFloat());

        // Calculate visualization bounds inside the inner box (no horizontal inset)
        const auto vizBounds = innerBoxBounds.reduced (0, 6);

        // Apply darkening if inactive
        if (!isActive)
        {
            g.setOpacity (Style::alphaInactive);
            g.setColour (TEXT_INACTIVE_COLOR);
        }
        else
        {
            g.setColour (TEXT_COLOR);
        }

        g.setFont (Style::fontComponent);

        // Draw the parameter name in top label area (fades out on hover)
        if (hoverAnimator.getAlpha() < 0.99f)
        {
            g.setOpacity (isActive ? (1.0f - hoverAnimator.getAlpha()) : Style::alphaInactive * (1.0f - hoverAnimator.getAlpha()));
            g.drawText (param->getName (15),
                topLabelArea,
                juce::Justification::centred,
                true);
        }

        // Draw the parameter value in top label area (fades in on hover)
        if (hoverAnimator.getAlpha() > 0.01f)
        {
            g.setOpacity (isActive ? hoverAnimator.getAlpha() : Style::alphaInactive * hoverAnimator.getAlpha());
            g.drawText (getParameterText(),
                topLabelArea,
                juce::Justification::centred,
                true);
        }

        g.setOpacity (isActive ? 1.0f : Style::alphaInactive);

        // Clip visualization to inner box
        g.saveState();
        g.reduceClipRegion (innerBoxBounds);

        // Draw the main visualization (implemented by derived classes)
        drawVisualization (g, vizBounds);

        // Draw modulation overlay
        drawModulationOverlay (g, vizBounds);

        g.restoreState();

        // Draw the active toggle button in the top-left if we have an active parameter
        if (activeParam != nullptr)
        {
            const auto toggleRect = getToggleBounds();

            g.setOpacity (1.0f);
            g.setColour (TERTIARY_COLOR);
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
        if (! inlineEditor.isEditing())
            setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        hoverAnimator.setHovered (true);
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        setMouseCursor (juce::MouseCursor::NormalCursor);
        hoverAnimator.setHovered (false);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (inlineEditor.isEditing())
            return;

        if (e.mods.isPopupMenu())
            return;

        if (e.getNumberOfClicks() > 1)
            return;

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

        // Store initial mouse position and parameter value for drag.
        // Defer beginChangeGesture until the user actually drags, so a
        // double-click doesn't leave an empty undo transaction behind.
        mouseDownY = e.y;
        initialParamValue = paramValue;
        isDragging = true;
        dragGestureStarted = false;
    }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        if (!isActive)
            return;

        if (modModeState != nullptr && modModeState->isModulationMode())
            return;

        if (! getLabelEditBounds().contains (e.getPosition()) || getToggleBounds().contains (e.getPosition()))
            return;

        auto initialText = getEditableParameterText();
        if (initialText.isEmpty())
            return;

        setMouseCursor (juce::MouseCursor::NormalCursor);
        inlineEditor.beginEdit (getLabelEditBounds().reduced (2),
            initialText,
            [this] (const juce::String& text)
            {
                commitTextToParameter (param, text);
            });
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (inlineEditor.isEditing())
            return;

        if (isModDragging && modModeState != nullptr)
        {
            auto sourceID = modModeState->getTargetSourceID();
            const auto bounds = getLocalBounds().withTrimmedTop (static_cast<int> (Style::labelHeight) + 4);

            float verticalDelta = (mouseDownY - e.y) / (bounds.getHeight() - padding) / 2.0f;
            float newDepth = juce::jlimit (-1.0f, 1.0f, modDragInitialDepth + verticalDelta);
            modModeState->setDepth (sourceID, paramDestID, newDepth);
            repaint();
            return;
        }

        if (!isDragging || !isActive)
            return;

        if (! dragGestureStarted)
        {
            if (undoManager != nullptr)
                undoManager->beginNewTransaction();
            param->beginChangeGesture();
            if (gestureCount != nullptr)
                ++(*gestureCount);
            dragGestureStarted = true;
        }

        const auto bounds = getLocalBounds().withTrimmedTop (static_cast<int> (Style::labelHeight) + 4);

        // Calculate vertical movement for the parameter
        // Moving up increases the value
        const float verticalDelta = (mouseDownY - e.y) / (bounds.getHeight() - padding) / 2.0f;
        const float newParamValue = juce::jlimit (0.0f, 1.0f, initialParamValue + verticalDelta);

        // Update parameter
        param->setValueNotifyingHost (newParamValue);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (inlineEditor.isEditing())
            return;

        if (e.mods.isPopupMenu() && e.mouseWasClicked() && getLocalBounds().contains (e.getPosition()))
        {
            if (modModeState != nullptr && modModeState->isModulationMode()
                && paramDestID.isNotEmpty() && isActive)
            {
                auto sourceID = modModeState->getTargetSourceID();
                if (modModeState->findSlotIndex (sourceID, paramDestID) >= 0)
                {
                    showModulationContextMenu (this, modModeState,
                        { { param->getName (15), sourceID, paramDestID } }, e.getScreenPosition());
                    return;
                }
            }

            if (activeParam != nullptr)
            {
                const auto toggleRect = getToggleBounds();
                if (toggleRect.contains (e.getPosition()) || e.mods.isPopupMenu())
                {
                    if (undoManager != nullptr)
                        undoManager->beginNewTransaction();
                    activeParam->beginChangeGesture();
                    activeParam->setValueNotifyingHost (isActive ? 0.0f : 1.0f);
                    activeParam->endChangeGesture();
                    return;
                }
            }
        }

        if (isModDragging)
        {
            if (gestureCount != nullptr)
                --(*gestureCount);
            isModDragging = false;
            return;
        }
        if (isDragging)
        {
            if (dragGestureStarted)
            {
                param->endChangeGesture();
                if (gestureCount != nullptr)
                    --(*gestureCount);
            }
        }
        isDragging = false;
        dragGestureStarted = false;
    }

protected:
    // Parameter pointers
    juce::RangedAudioParameter* param;
    juce::AudioParameterBool* activeParam;

    // Active state
    bool isActive = true;

    // Hover animation
    HoverAnimator hoverAnimator { *this };
    InlineParameterEditor inlineEditor { *this };

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

    virtual juce::String getEditableParameterText() const
    {
        return InlineParameterEditUtils::extractEditableText (getParameterText());
    }

    virtual float parseEditedParameterText (juce::RangedAudioParameter* targetParam, const juce::String& text) const
    {
        return InlineParameterEditUtils::parseNormalizedValue (targetParam, text, getParameterText());
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

    juce::Rectangle<int> getTopLabelBounds() const
    {
        auto innerArea = getLocalBounds().reduced (kOuterPad);
        return innerArea.removeFromTop (static_cast<int> (Style::labelHeight));
    }

    juce::Rectangle<int> getInnerBoxBounds() const
    {
        auto innerArea = getLocalBounds().reduced (kOuterPad);
        innerArea.removeFromTop (static_cast<int> (Style::labelHeight));
        return innerArea.reduced (kOuterPad);
    }

    juce::Rectangle<int> getToggleBounds() const
    {
        if (activeParam == nullptr)
            return {};

        constexpr int toggleSize = 12;
        const int toggleY = kOuterPad + (static_cast<int> (Style::labelHeight) - toggleSize) / 2;
        return { kOuterPad + 2, toggleY, toggleSize, toggleSize };
    }

    juce::Rectangle<int> getLabelEditBounds() const
    {
        return getTopLabelBounds();
    }

    void commitTextToParameter (juce::RangedAudioParameter* targetParam, const juce::String& text)
    {
        if (targetParam == nullptr)
            return;

        if (undoManager != nullptr)
            undoManager->beginNewTransaction();

        targetParam->beginChangeGesture();
        if (gestureCount != nullptr)
            ++(*gestureCount);
        targetParam->setValueNotifyingHost (parseEditedParameterText (targetParam, text));
        targetParam->endChangeGesture();
        if (gestureCount != nullptr)
            --(*gestureCount);
    }

private:
    static constexpr int kOuterPad = 4;

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

        g.setColour (getModColor (sourceID).withAlpha (Style::alphaMod));
        g.drawHorizontalLine (static_cast<int> (modY), static_cast<float> (bounds.getX()), static_cast<float> (bounds.getRight()));

        if (bipolar)
        {
            float ghostValue = juce::jlimit (0.0f, 1.0f, paramValue - depth);
            float ghostY = bounds.getBottom() - ghostValue * bounds.getHeight();
            g.setColour (getModColor (sourceID).withAlpha (Style::alphaModGhost));
            g.drawHorizontalLine (static_cast<int> (ghostY), static_cast<float> (bounds.getX()), static_cast<float> (bounds.getRight()));
        }
    }

    // Mouse drag tracking
    bool isDragging = false;
    bool dragGestureStarted = false;
    bool isModDragging = false;
    int mouseDownY = 0;
    float initialParamValue = 0.0f;
    float modDragInitialDepth = 0.0f;
};
