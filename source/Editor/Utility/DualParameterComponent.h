//
// Created by Tyler Teuber on 5/10/25.
//

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../../Theme.h"
#include "ModulationModeState.h"
#include "ModulationContextMenu.h"
#include "HoverAnimator.h"
#include "PaintHelpers.h"
#include "UIContext.h"

// Base Component Class
class DualParameterComponent : public juce::Component,
                               private juce::AudioProcessorParameter::Listener
{
public:
    DualParameterComponent (juce::RangedAudioParameter* param1,
        juce::RangedAudioParameter* param2,
        juce::AudioParameterBool* activeParam = nullptr,
        const UIContext& ctx = {},
        const juce::String& param1DestID = {},
        const juce::String& param2DestID = {},
        const juce::String& label = {})
        : param1 (param1),
          param2 (param2),
          activeParam (activeParam),
          componentLabel (label),
          undoManager (ctx.undoManager),
          gestureCount (ctx.gestureCount),
          modModeState (ctx.modModeState),
          param1DestID (param1DestID),
          param2DestID (param2DestID)
    {
        // Register as a listener for both parameters
        param1->addListener (this);
        param2->addListener (this);

        // Set initial values from parameters
        param1Value = param1->getValue();
        param2Value = param2->getValue();

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

    ~DualParameterComponent() override
    {
        // Remove listeners
        param1->removeListener (this);
        param2->removeListener (this);

        if (activeParam != nullptr)
            activeParam->removeListener (this);
    }

    void paint (juce::Graphics& g) override
    {
        // Fill with parent background color
        g.fillAll (PRIMARY_COLOR);

        const int labelH = static_cast<int> (Style::labelHeight);

        // Outer box: bordered container with SECONDARY_COLOR background
        auto outerBounds = getLocalBounds();
        PaintHelpers::drawComponentBox (g, outerBounds.toFloat());

        // Layout inside the outer box: top label, inner box, bottom label
        auto innerArea = outerBounds.reduced (4); // padding inside outer box
        auto topLabelArea = innerArea.removeFromTop (labelH);
        auto bottomLabelArea = innerArea.removeFromBottom (labelH);
        auto innerBoxBounds = innerArea.reduced (4);

        // Inner box: darker fill for visualization area
        PaintHelpers::drawInnerBox (g, innerBoxBounds.toFloat());

        // Calculate visualization bounds inside the inner box (no horizontal inset)
        const auto vizBounds = innerBoxBounds.reduced (0, static_cast<int> (Style::vizInset));

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

        // Clip to inner box so visualizations don't bleed outside
        g.saveState();
        g.reduceClipRegion (innerBoxBounds);

        // Draw the main visualization
        drawVisualization (g, vizBounds);

        // Draw modulation overlay if in mod mode
        drawModulationOverlay (g, vizBounds);

        g.restoreState();

        // Draw text indicators
        g.setFont (Style::fontComponent);

        if (!isActive)
            g.setColour (TEXT_INACTIVE_COLOR);
        else
            g.setColour (TEXT_COLOR);

        // Draw top label (component name, fades out on hover)
        if (componentLabel.isNotEmpty() && hoverAnimator.getAlpha() < 0.99f)
        {
            g.setOpacity ((1.0f - hoverAnimator.getAlpha()) * (isActive ? 1.0f : Style::alphaInactive));
            g.drawText (componentLabel,
                topLabelArea,
                juce::Justification::centred,
                true);
        }

        // Draw parameter texts with fading opacity (visible when hovering)
        if (hoverAnimator.getAlpha() > 0.01f)
        {
            g.setOpacity (hoverAnimator.getAlpha() * (isActive ? 1.0f : Style::alphaInactive));

            // Display parameter 1 text (bottom label area, below inner box)
            g.drawText (getParam1Text(),
                bottomLabelArea,
                juce::Justification::centred,
                true);

            // Display parameter 2 text (top label area, above inner box)
            g.drawText (getParam2Text(),
                topLabelArea,
                juce::Justification::centred,
                true);
        }

    }

    void resized() override
    {
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        // Modulation context menu on right-click in mod mode
        if (e.mods.isRightButtonDown()
            && modModeState != nullptr && modModeState->isModulationMode()
            && isActive)
        {
            auto sourceID = modModeState->getTargetSourceID();
            std::vector<ModulationContextMenu::ParamInfo> entries;

            if (param1DestID.isNotEmpty() && modModeState->findSlotIndex (sourceID, param1DestID) >= 0)
                entries.push_back ({ param1->getName (15), sourceID, param1DestID });
            if (param2DestID.isNotEmpty() && modModeState->findSlotIndex (sourceID, param2DestID) >= 0)
                entries.push_back ({ param2->getName (15), sourceID, param2DestID });

            if (!entries.empty())
            {
                showModulationContextMenu (this, modModeState, std::move (entries), e.getScreenPosition());
                return;
            }
        }

        // Toggle active state on right-click
        if (activeParam != nullptr && e.mods.isRightButtonDown())
        {
            if (undoManager != nullptr)
                undoManager->beginNewTransaction();
            activeParam->beginChangeGesture();
            activeParam->setValueNotifyingHost (isActive ? 0.0f : 1.0f);
            activeParam->endChangeGesture();
            return;
        }

        // Modulation mode handling
        if (modModeState != nullptr && modModeState->isModulationMode() && !e.mods.isShiftDown())
        {
            // Block interaction if no dest IDs configured
            if (param1DestID.isEmpty() && param2DestID.isEmpty())
                return;

            if (!isActive)
                return;

            modDragSourceID = modModeState->getTargetSourceID();

            if (auto* um = modModeState->getUndoManager())
                um->beginNewTransaction();
            if (gestureCount != nullptr)
                ++(*gestureCount);

            mouseDownX = e.x;
            mouseDownY = e.y;
            isModDragging = true;
            modParam1Engaged = false;
            modParam2Engaged = false;
            return;
        }

        // Only process drag if the component is active
        if (!isActive)
            return;

        // Store initial mouse position and parameter values for drag
        mouseDownX = e.x;
        mouseDownY = e.y;
        initialParam1Value = param1Value;
        initialParam2Value = param2Value;

        if (undoManager != nullptr)
            undoManager->beginNewTransaction();
        param1->beginChangeGesture();
        param2->beginChangeGesture();
        if (gestureCount != nullptr)
            ++(*gestureCount);

        isDragging = true;
        param1Engaged = false;
        param2Engaged = false;
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (isModDragging && modModeState != nullptr)
        {
            const auto& sourceID = modDragSourceID;
            const int labelH = static_cast<int> (Style::labelHeight);
            const auto bounds = getLocalBounds().reduced (4).withTrimmedTop (labelH).withTrimmedBottom (labelH);
            const int absX = std::abs (e.x - mouseDownX);
            const int absY = std::abs (e.y - mouseDownY);

            const bool wasModParam1Engaged = modParam1Engaged;
            const bool wasModParam2Engaged = modParam2Engaged;
            const bool wasModEitherEngaged = wasModParam1Engaged || wasModParam2Engaged;

            if (!wasModParam1Engaged && param1DestID.isNotEmpty()
                && absY >= (wasModEitherEngaged ? kSecondaryThreshold : kPrimaryThreshold))
            {
                int slot = modModeState->getOrCreateSlot (sourceID, param1DestID);
                if (slot >= 0)
                {
                    modParam1Engaged = true;
                    modParam1RefY = e.y;
                    modDragInitialDepth1 = modModeState->getDepth (sourceID, param1DestID);
                }
            }
            if (!wasModParam2Engaged && param2DestID.isNotEmpty()
                && absX >= (wasModEitherEngaged ? kSecondaryThreshold : kPrimaryThreshold))
            {
                int slot = modModeState->getOrCreateSlot (sourceID, param2DestID);
                if (slot >= 0)
                {
                    modParam2Engaged = true;
                    modParam2RefX = e.x;
                    modDragInitialDepth2 = modModeState->getDepth (sourceID, param2DestID);
                }
            }

            if (modParam1Engaged && param1DestID.isNotEmpty())
            {
                float verticalDelta = (modParam1RefY - e.y) / (bounds.getHeight() - padding);
                float newDepth = juce::jlimit (-1.0f, 1.0f, modDragInitialDepth1 + verticalDelta);
                modModeState->setDepth (sourceID, param1DestID, newDepth);
            }
            if (modParam2Engaged && param2DestID.isNotEmpty())
            {
                float horizontalDelta = (e.x - modParam2RefX) / (bounds.getWidth() - padding);
                float newDepth = juce::jlimit (-1.0f, 1.0f, modDragInitialDepth2 + horizontalDelta);
                modModeState->setDepth (sourceID, param2DestID, newDepth);
            }

            repaint();
            return;
        }

        if (!isDragging || !isActive)
            return;

        const int labelH = static_cast<int> (Style::labelHeight);
        const auto bounds = getLocalBounds().reduced (4).withTrimmedTop (labelH).withTrimmedBottom (labelH);
        const int absX = std::abs (e.x - mouseDownX);
        const int absY = std::abs (e.y - mouseDownY);

        const bool wasParam1Engaged = param1Engaged;
        const bool wasParam2Engaged = param2Engaged;
        const bool wasEitherEngaged = wasParam1Engaged || wasParam2Engaged;

        if (!wasParam1Engaged && absY >= (wasEitherEngaged ? kSecondaryThreshold : kPrimaryThreshold))
        {
            param1Engaged = true;
            param1RefY = e.y;
            initialParam1Value = param1Value;
        }
        if (!wasParam2Engaged && absX >= (wasEitherEngaged ? kSecondaryThreshold : kPrimaryThreshold))
        {
            param2Engaged = true;
            param2RefX = e.x;
            initialParam2Value = param2Value;
        }

        if (param1Engaged)
        {
            const float verticalDelta = (param1RefY - e.y) / (bounds.getHeight() - padding);
            param1->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, initialParam1Value + verticalDelta));
        }
        if (param2Engaged)
        {
            const float horizontalDelta = (e.x - param2RefX) / (bounds.getWidth() - padding);
            param2->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, initialParam2Value + horizontalDelta));
        }
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
            param1->endChangeGesture();
            param2->endChangeGesture();
            if (gestureCount != nullptr)
                --(*gestureCount);
        }
        isDragging = false;
    }

    void mouseEnter (const juce::MouseEvent&) override
    {
        setMouseCursor (juce::MouseCursor::CrosshairCursor);
        hoverAnimator.setHovered (true);
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        setMouseCursor (juce::MouseCursor::NormalCursor);
        hoverAnimator.setHovered (false);
    }

protected:
    // Parameter pointers
    juce::RangedAudioParameter* param1;
    juce::RangedAudioParameter* param2;
    juce::AudioParameterBool* activeParam;

    // Label and hover animation
    juce::String componentLabel;
    HoverAnimator hoverAnimator { *this };

    // Active state
    bool isActive = true;

    // Current parameter values
    float param1Value = 0.0f;
    float param2Value = 0.0f;

    juce::Colour getDrawColor() const { return isActive ? TEXT_COLOR : TEXT_INACTIVE_COLOR; }

    float padding = 0.5f; // Padding for drawing

    juce::UndoManager* undoManager = nullptr;
    std::atomic<int>* gestureCount = nullptr;

    // Modulation mode
    ModulationModeState* modModeState = nullptr;
    juce::String param1DestID;
    juce::String param2DestID;

    // Virtual method to be implemented by derived classes
    virtual void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const = 0;

    // Virtual method for drawing visualization with explicit values (for mod overlay)
    virtual void drawVisualizationWithValues (juce::Graphics& g,
        const juce::Rectangle<int>& bounds, float p1, float p2) const
    {
        juce::ignoreUnused (p1, p2);
        drawVisualization (g, bounds);
    }

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
        else if (parameterIndex == param2->getParameterIndex())
            param2Value = newValue;
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
        if (param1DestID.isEmpty() && param2DestID.isEmpty())
            return;

        auto sourceID = modModeState->getTargetSourceID();

        float depth1 = param1DestID.isNotEmpty() ? modModeState->getDepth (sourceID, param1DestID) : 0.0f;
        float depth2 = param2DestID.isNotEmpty() ? modModeState->getDepth (sourceID, param2DestID) : 0.0f;


        bool bipolar1 = param1DestID.isNotEmpty() && modModeState->isBipolar (sourceID, param1DestID);
        bool bipolar2 = param2DestID.isNotEmpty() && modModeState->isBipolar (sourceID, param2DestID);

        // Draw cyan overlay at param + depth
        float modP1 = juce::jlimit (0.0f, 1.0f, param1Value + depth1);
        float modP2 = juce::jlimit (0.0f, 1.0f, param2Value + depth2);

        g.setColour (getModColor (sourceID).withAlpha (Style::alphaMod));
        drawVisualizationWithValues (g, bounds, modP1, modP2);

        // Draw faint ghost for bipolar (opposite direction)
        if (bipolar1 || bipolar2)
        {
            float ghostP1 = bipolar1 ? juce::jlimit (0.0f, 1.0f, param1Value - depth1) : modP1;
            float ghostP2 = bipolar2 ? juce::jlimit (0.0f, 1.0f, param2Value - depth2) : modP2;

            g.setColour (getModColor (sourceID).withAlpha (Style::alphaModGhost));
            drawVisualizationWithValues (g, bounds, ghostP1, ghostP2);
        }
    }

    // Mouse drag tracking
    bool isDragging = false;
    bool isModDragging = false;
    juce::String modDragSourceID;
    int mouseDownX = 0;
    int mouseDownY = 0;
    float initialParam1Value = 0.0f;
    float initialParam2Value = 0.0f;
    float modDragInitialDepth1 = 0.0f;
    float modDragInitialDepth2 = 0.0f;

    // Axis-locking dead zone
    bool param1Engaged = false;
    bool param2Engaged = false;
    int param1RefY = 0;
    int param2RefX = 0;

    bool modParam1Engaged = false;
    bool modParam2Engaged = false;
    int modParam1RefY = 0;
    int modParam2RefX = 0;

    static constexpr int kPrimaryThreshold = 4;
    static constexpr int kSecondaryThreshold = 12;
};
