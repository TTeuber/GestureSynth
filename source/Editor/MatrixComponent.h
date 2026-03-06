#pragma once

#include "../PluginProcessor.h"
#include "../Theme.h"
#include "../Utility/AtomicHelpers.h"
#include "Utility/AnimationFrameSource.h"
#include "Utility/DepthSlider.h"
#include "Utility/ModulationIconDrawing.h"
#include <juce_gui_basics/juce_gui_basics.h>

class MatrixComponent final : public juce::Component, public juce::ValueTree::Listener, public AnimationFrameSource::Listener
{
public:
    explicit MatrixComponent (juce::ValueTree& mt, std::atomic<float>* srcOutputs = nullptr,
        juce::UndoManager* um = nullptr, std::atomic<int>* gc = nullptr,
        AnimationFrameSource* animSource = nullptr)
        : modTree (mt), sourceOutputs (srcOutputs), undoManager (um), gestureCount (gc),
          animSource (animSource)
    {
        modTree.addListener (this);

        for (int i = 0; i < 16 && i < modTree.getNumChildren(); ++i)
        {
            auto child = modTree.getChild (i);
            auto& row = rows[i];

            // Source ComboBox
            for (int s = 0; s < PluginProcessor::modSourceIDs.size(); ++s)
                row.sourceBox.addItem (PluginProcessor::modSourceIDs[s], s + 1);
            row.sourceBox.setSelectedId (
                PluginProcessor::modSourceIDs.indexOf (child.getProperty ("source").toString()) + 1,
                juce::dontSendNotification);
            row.sourceBox.setColour (juce::ComboBox::backgroundColourId, SECONDARY_COLOR);
            row.sourceBox.setColour (juce::ComboBox::textColourId, TEXT_COLOR);
            row.sourceBox.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
            addAndMakeVisible (row.sourceBox);

            // Depth slider
            row.depthSlider.setValue (static_cast<float> (child.getProperty ("depth")));
            row.depthSlider.setBipolar (static_cast<bool> (child.getProperty ("isBipolar")));
            addAndMakeVisible (row.depthSlider);

            // Bipolar button
            row.bipolarButton.setBipolar (static_cast<bool> (child.getProperty ("isBipolar")));
            addAndMakeVisible (row.bipolarButton);

            // Destination ComboBox
            for (int d = 0; d < PluginProcessor::modDestIDs.size(); ++d)
                row.destBox.addItem (PluginProcessor::modDestIDs[d], d + 1);
            row.destBox.setSelectedId (
                PluginProcessor::modDestIDs.indexOf (child.getProperty ("destination").toString()) + 1,
                juce::dontSendNotification);
            row.destBox.setColour (juce::ComboBox::backgroundColourId, SECONDARY_COLOR);
            row.destBox.setColour (juce::ComboBox::textColourId, TEXT_COLOR);
            row.destBox.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
            addAndMakeVisible (row.destBox);

            // Bypass button
            row.bypassButton.setBypassed (static_cast<bool> (child.getProperty ("bypassed")));
            row.bypassButton.setTooltip ("Bypass modulation");
            addAndMakeVisible (row.bypassButton);

            // Clear button
            row.clearButton.setTooltip ("Clear modulation");
            addAndMakeVisible (row.clearButton);

            // Wire callbacks
            row.sourceBox.onChange = [this, i]()
            {
                if (undoManager != nullptr)
                    undoManager->beginNewTransaction();
                auto selectedSource = PluginProcessor::modSourceIDs[rows[i].sourceBox.getSelectedId() - 1];
                modTree.getChild (i).setProperty ("source", selectedSource, undoManager);
            };

            row.depthSlider.onDragStart = [this]()
            {
                if (undoManager != nullptr)
                    undoManager->beginNewTransaction();
                if (gestureCount != nullptr)
                    ++(*gestureCount);
            };

            row.depthSlider.onDragEnd = [this]()
            {
                if (gestureCount != nullptr)
                    --(*gestureCount);
            };

            row.depthSlider.onValueChange = [this, i]()
            {
                modTree.getChild (i).setProperty ("depth", rows[i].depthSlider.getValue(), undoManager);
            };

            row.bipolarButton.onClick = [this, i]()
            {
                if (undoManager != nullptr)
                    undoManager->beginNewTransaction();
                auto& btn = rows[i].bipolarButton;
                btn.setBipolar (!btn.isBipolar());
                rows[i].depthSlider.setBipolar (btn.isBipolar());
                modTree.getChild (i).setProperty ("isBipolar", btn.isBipolar(), undoManager);
            };

            row.destBox.onChange = [this, i]()
            {
                if (undoManager != nullptr)
                    undoManager->beginNewTransaction();
                auto selectedDest = PluginProcessor::modDestIDs[rows[i].destBox.getSelectedId() - 1];
                modTree.getChild (i).setProperty ("destination", selectedDest, undoManager);
            };

            row.bypassButton.onClick = [this, i]()
            {
                if (undoManager != nullptr)
                    undoManager->beginNewTransaction();
                auto child = modTree.getChild (i);
                bool current = static_cast<bool> (child.getProperty ("bypassed"));
                child.setProperty ("bypassed", !current, undoManager);
            };

            row.clearButton.onClick = [this, i]()
            {
                if (undoManager != nullptr)
                    undoManager->beginNewTransaction();
                auto child = modTree.getChild (i);
                child.setProperty ("source", "None", undoManager);
                child.setProperty ("destination", "None", undoManager);
                child.setProperty ("depth", 0.0f, undoManager);
                child.setProperty ("isBipolar", false, undoManager);
                child.setProperty ("bypassed", false, undoManager);
            };
        }

        if (sourceOutputs != nullptr && animSource != nullptr)
            animSource->addListener (this, AnimationFrameSource::Rate::Hz30);
    }

    ~MatrixComponent() override
    {
        if (animSource != nullptr)
            animSource->removeListener (this);
        modTree.removeListener (this);
    }

    void onAnimationFrame() override
    {
        if (sourceOutputs == nullptr)
            return;

        for (int i = 0; i < 16; ++i)
            rows[i].depthSlider.setSourceValue (AtomicHelpers::paramLoad (sourceOutputs[i]));
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (SECONDARY_COLOR);
    }

    void paintOverChildren (juce::Graphics& g) override
    {
        const int numRows = 16;
        const int rowHeight = getHeight() / numRows;
        g.setColour (TEXT_INACTIVE_COLOR.withAlpha (0.3f));
        for (int i = 1; i < numRows; ++i)
            g.drawHorizontalLine (i * rowHeight, 0.0f, static_cast<float> (getWidth()));
    }

    void resized() override
    {
        auto area = getLocalBounds();
        const int numRows = 16;
        const int rowHeight = area.getHeight() / numRows;

        for (int i = 0; i < numRows; ++i)
        {
            auto rowArea = area.removeFromTop (rowHeight);

            auto& row = rows[i];

            const int srcWidth = rowArea.getWidth() / 4;
            const int dstWidth = rowArea.getWidth() / 4;
            const int btnSize = 24;
            const int vPad = (rowArea.getHeight() - btnSize) / 2;

            row.sourceBox.setBounds (rowArea.removeFromLeft (srcWidth));
            row.clearButton.setBounds (rowArea.removeFromRight (btnSize).reduced (0, vPad));
            row.bypassButton.setBounds (rowArea.removeFromRight (btnSize).reduced (0, vPad));
            row.destBox.setBounds (rowArea.removeFromRight (dstWidth));
            row.bipolarButton.setBounds (rowArea.removeFromRight (btnSize).reduced (0, vPad));
            row.depthSlider.setBounds (rowArea);
        }
    }

    void valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override
    {
        auto parent = treeWhosePropertyHasChanged.getParent();
        if (!parent.isValid() || parent != modTree)
            return;

        const int index = modTree.indexOf (treeWhosePropertyHasChanged);
        if (index < 0 || index >= 16)
            return;

        auto& row = rows[index];

        if (property.toString() == "source")
        {
            const int id = PluginProcessor::modSourceIDs.indexOf (
                treeWhosePropertyHasChanged.getProperty ("source").toString()) + 1;
            if (id > 0)
                row.sourceBox.setSelectedId (id, juce::dontSendNotification);
        }
        else if (property.toString() == "destination")
        {
            const int id = PluginProcessor::modDestIDs.indexOf (
                treeWhosePropertyHasChanged.getProperty ("destination").toString()) + 1;
            if (id > 0)
                row.destBox.setSelectedId (id, juce::dontSendNotification);
        }
        else if (property.toString() == "depth")
        {
            row.depthSlider.setValue (
                static_cast<float> (treeWhosePropertyHasChanged.getProperty ("depth")));
        }
        else if (property.toString() == "isBipolar")
        {
            bool bp = static_cast<bool> (treeWhosePropertyHasChanged.getProperty ("isBipolar"));
            row.bipolarButton.setBipolar (bp);
            row.depthSlider.setBipolar (bp);
        }
        else if (property.toString() == "bypassed")
        {
            row.bypassButton.setBypassed (
                static_cast<bool> (treeWhosePropertyHasChanged.getProperty ("bypassed")));
        }
    }

    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override {}
    void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override {}

private:
    juce::ValueTree& modTree;
    std::atomic<float>* sourceOutputs = nullptr;
    juce::UndoManager* undoManager = nullptr;
    std::atomic<int>* gestureCount = nullptr;
    AnimationFrameSource* animSource = nullptr;
    juce::TooltipWindow tooltipWindow { this, 500 };

    // ---------------------------------------------------------------------------------
    // Bipolar toggle button — draws <--> (bipolar) or --> (unipolar)
    // ---------------------------------------------------------------------------------
    class BipolarButton : public juce::Component
    {
    public:
        BipolarButton() { setRepaintsOnMouseActivity (true); }

        void paint (juce::Graphics& g) override
        {
            const auto bounds = getLocalBounds().toFloat();

            auto bg = isMouseOver() ? SECONDARY_COLOR.brighter (0.15f) : SECONDARY_COLOR;
            g.setColour (bg);
            g.fillRoundedRectangle (bounds, 4.0f);
            g.setColour (TEXT_COLOR.withAlpha (0.3f));
            g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);

            ModulationIcons::drawBipolarIcon (g, bounds.reduced (5.0f), bipolar);
        }

        void mouseUp (const juce::MouseEvent& e) override
        {
            if (getLocalBounds().contains (e.x, e.y) && onClick)
                onClick();
        }

        bool isBipolar() const { return bipolar; }
        void setBipolar (bool b) { bipolar = b; repaint(); }

        std::function<void()> onClick;

    private:
        bool bipolar = false;
    };

    // ---------------------------------------------------------------------------------
    // Bypass button — circle with diagonal line (prohibition icon)
    // ---------------------------------------------------------------------------------
    class BypassButton : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        BypassButton() { setRepaintsOnMouseActivity (true); }

        void paint (juce::Graphics& g) override
        {
            const auto bounds = getLocalBounds().toFloat();

            auto bg = isMouseOver() ? SECONDARY_COLOR.brighter (0.15f) : SECONDARY_COLOR;
            g.setColour (bg);
            g.fillRoundedRectangle (bounds, 4.0f);

            ModulationIcons::drawBypassIcon (g, bounds.reduced (5.0f), bypassed);
        }

        void mouseUp (const juce::MouseEvent& e) override
        {
            if (getLocalBounds().contains (e.x, e.y) && onClick)
                onClick();
        }

        bool isBypassed() const { return bypassed; }
        void setBypassed (bool b) { bypassed = b; repaint(); }

        std::function<void()> onClick;

    private:
        bool bypassed = false;
    };

    // ---------------------------------------------------------------------------------
    // Clear button — X icon
    // ---------------------------------------------------------------------------------
    class ClearButton : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        ClearButton() { setRepaintsOnMouseActivity (true); }

        void paint (juce::Graphics& g) override
        {
            const auto bounds = getLocalBounds().toFloat();

            auto bg = isMouseOver() ? SECONDARY_COLOR.brighter (0.15f) : SECONDARY_COLOR;
            g.setColour (bg);
            g.fillRoundedRectangle (bounds, 4.0f);

            ModulationIcons::drawClearIcon (g, bounds.reduced (6.0f), isMouseOver());
        }

        void mouseUp (const juce::MouseEvent& e) override
        {
            if (getLocalBounds().contains (e.x, e.y) && onClick)
                onClick();
        }

        std::function<void()> onClick;
    };

    struct SlotRow
    {
        juce::ComboBox sourceBox;
        DepthSlider    depthSlider;
        BipolarButton  bipolarButton;
        juce::ComboBox destBox;
        BypassButton   bypassButton;
        ClearButton    clearButton;
    };
    std::array<SlotRow, 16> rows;
};
