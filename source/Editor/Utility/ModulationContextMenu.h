//
// Context menu popup for modulation controls on parameter components
//

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Theme.h"
#include "ModulationModeState.h"
#include "ModulationIconDrawing.h"

class ModulationContextMenu : public juce::Component
{
public:
    struct ParamInfo
    {
        juce::String name;
        juce::String sourceID;
        juce::String destID;
    };

    ModulationContextMenu (ModulationModeState* state, std::vector<ParamInfo> params)
        : modModeState (state), entries (std::move (params))
    {
        const int w = 160;
        const int sectionH = 32;
        const int dividers = entries.size() > 1 ? static_cast<int> (entries.size()) - 1 : 0;
        setSize (w, static_cast<int> (entries.size()) * sectionH + dividers);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (SECONDARY_COLOR);

        const int sectionH = 32;
        const int iconSize = 20;
        const int iconPad = 4;

        for (int i = 0; i < static_cast<int> (entries.size()); ++i)
        {
            const int yOff = i * (sectionH + (i > 0 ? 1 : 0));

            // Divider
            if (i > 0)
            {
                g.setColour (TEXT_INACTIVE_COLOR.withAlpha (0.3f));
                g.drawHorizontalLine (yOff - 1, 0.0f, static_cast<float> (getWidth()));
            }

            auto sectionBounds = juce::Rectangle<int> (0, yOff, getWidth(), sectionH);

            // Parameter name
            g.setColour (TEXT_COLOR);
            g.setFont (12.0f);
            g.drawText (entries[i].name, sectionBounds.withTrimmedLeft (6).withTrimmedRight (iconSize * 3 + iconPad * 3),
                        juce::Justification::centredLeft, true);

            // Icon positions (right-aligned): bipolar, bypass, clear
            auto iconArea = sectionBounds.removeFromRight (iconSize * 3 + iconPad * 3);
            iconArea.removeFromLeft (iconPad);

            auto bipolarBounds = iconArea.removeFromLeft (iconSize).toFloat().reduced (2.0f);
            iconArea.removeFromLeft (iconPad);
            auto bypassBounds = iconArea.removeFromLeft (iconSize).toFloat().reduced (2.0f);
            iconArea.removeFromLeft (iconPad);
            auto clearBounds = iconArea.removeFromLeft (iconSize).toFloat().reduced (3.0f);

            // Highlight hovered button
            if (hoveredSection == i && hoveredButton >= 0)
            {
                juce::Rectangle<float> hoverBounds;
                if (hoveredButton == 0) hoverBounds = bipolarBounds.expanded (2.0f);
                else if (hoveredButton == 1) hoverBounds = bypassBounds.expanded (2.0f);
                else hoverBounds = clearBounds.expanded (3.0f);

                g.setColour (SECONDARY_COLOR.brighter (0.15f));
                g.fillRoundedRectangle (hoverBounds, 3.0f);
            }

            bool bipolar = modModeState->isBipolar (entries[i].sourceID, entries[i].destID);
            bool bypassed = modModeState->isBypassed (entries[i].sourceID, entries[i].destID);

            ModulationIcons::drawBipolarIcon (g, bipolarBounds, bipolar);
            ModulationIcons::drawBypassIcon (g, bypassBounds, bypassed);
            ModulationIcons::drawClearIcon (g, clearBounds, hoveredSection == i && hoveredButton == 2);
        }
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        updateHover (e.getPosition());
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        hoveredSection = -1;
        hoveredButton = -1;
        repaint();
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (!getLocalBounds().contains (e.getPosition()))
            return;

        int section, button;
        hitTest (e.getPosition(), section, button);

        if (section < 0 || button < 0)
            return;

        auto& entry = entries[section];

        if (button == 0) // bipolar
        {
            modModeState->toggleBipolar (entry.sourceID, entry.destID);
            repaint();
        }
        else if (button == 1) // bypass
        {
            bool current = modModeState->isBypassed (entry.sourceID, entry.destID);
            modModeState->setBypassed (entry.sourceID, entry.destID, !current);
            repaint();
        }
        else if (button == 2) // clear
        {
            modModeState->clearSlot (entry.sourceID, entry.destID);
            if (auto* callout = findParentComponentOfClass<juce::CallOutBox>())
                callout->dismiss();
        }
    }

private:
    void updateHover (juce::Point<int> pos)
    {
        int section, button;
        hitTest (pos, section, button);

        if (section != hoveredSection || button != hoveredButton)
        {
            hoveredSection = section;
            hoveredButton = button;
            repaint();
        }
    }

    void hitTest (juce::Point<int> pos, int& outSection, int& outButton) const
    {
        outSection = -1;
        outButton = -1;

        const int sectionH = 32;
        const int iconSize = 20;
        const int iconPad = 4;

        for (int i = 0; i < static_cast<int> (entries.size()); ++i)
        {
            const int yOff = i * (sectionH + (i > 0 ? 1 : 0));
            auto sectionBounds = juce::Rectangle<int> (0, yOff, getWidth(), sectionH);

            if (!sectionBounds.contains (pos))
                continue;

            outSection = i;

            auto iconArea = sectionBounds;
            iconArea = iconArea.removeFromRight (iconSize * 3 + iconPad * 3);
            iconArea.removeFromLeft (iconPad);

            auto bipolarRect = iconArea.removeFromLeft (iconSize);
            iconArea.removeFromLeft (iconPad);
            auto bypassRect = iconArea.removeFromLeft (iconSize);
            iconArea.removeFromLeft (iconPad);
            auto clearRect = iconArea.removeFromLeft (iconSize);

            if (bipolarRect.contains (pos)) outButton = 0;
            else if (bypassRect.contains (pos)) outButton = 1;
            else if (clearRect.contains (pos)) outButton = 2;
            return;
        }
    }

    ModulationModeState* modModeState;
    std::vector<ParamInfo> entries;
    int hoveredSection = -1;
    int hoveredButton = -1;
};

//
// Helper to show the context menu as a CallOutBox
//
inline void showModulationContextMenu (juce::Component* target,
                                       ModulationModeState* modModeState,
                                       std::vector<ModulationContextMenu::ParamInfo> params,
                                       juce::Point<int> screenPos)
{
    juce::ignoreUnused (target);
    auto content = std::make_unique<ModulationContextMenu> (modModeState, std::move (params));
    juce::Rectangle<int> targetArea (screenPos.x, screenPos.y, 1, 1);

    juce::CallOutBox::launchAsynchronously (std::move (content), targetArea, nullptr);
}
