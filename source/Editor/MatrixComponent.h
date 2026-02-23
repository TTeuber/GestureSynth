#pragma once

#include "../PluginProcessor.h"
#include "../Theme.h"
#include "Utility/DepthSlider.h"
#include <juce_gui_basics/juce_gui_basics.h>

class MatrixComponent final : public juce::Component, public juce::ValueTree::Listener, public juce::Timer
{
public:
    explicit MatrixComponent (juce::ValueTree& mt, std::atomic<float>* srcOutputs = nullptr)
        : modTree (mt), sourceOutputs (srcOutputs)
    {
        modTree.addListener (this);

        for (int i = 0; i < 12 && i < modTree.getNumChildren(); ++i)
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
                auto selectedSource = PluginProcessor::modSourceIDs[rows[i].sourceBox.getSelectedId() - 1];
                modTree.getChild (i).setProperty ("source", selectedSource, nullptr);
            };

            row.depthSlider.onValueChange = [this, i]()
            {
                modTree.getChild (i).setProperty ("depth", rows[i].depthSlider.getValue(), nullptr);
            };

            row.bipolarButton.onClick = [this, i]()
            {
                auto& btn = rows[i].bipolarButton;
                btn.setBipolar (!btn.isBipolar());
                rows[i].depthSlider.setBipolar (btn.isBipolar());
                modTree.getChild (i).setProperty ("isBipolar", btn.isBipolar(), nullptr);
            };

            row.destBox.onChange = [this, i]()
            {
                auto selectedDest = PluginProcessor::modDestIDs[rows[i].destBox.getSelectedId() - 1];
                modTree.getChild (i).setProperty ("destination", selectedDest, nullptr);
            };

            row.bypassButton.onClick = [this, i]()
            {
                auto child = modTree.getChild (i);
                bool current = static_cast<bool> (child.getProperty ("bypassed"));
                child.setProperty ("bypassed", !current, nullptr);
            };

            row.clearButton.onClick = [this, i]()
            {
                auto child = modTree.getChild (i);
                child.setProperty ("source", "None", nullptr);
                child.setProperty ("destination", "None", nullptr);
                child.setProperty ("depth", 0.0f, nullptr);
                child.setProperty ("isBipolar", false, nullptr);
                child.setProperty ("bypassed", false, nullptr);
            };
        }

        if (sourceOutputs != nullptr)
            startTimerHz (30);
    }

    ~MatrixComponent() override
    {
        stopTimer();
        modTree.removeListener (this);
    }

    void timerCallback() override
    {
        if (sourceOutputs == nullptr)
            return;

        for (int i = 0; i < 12; ++i)
            rows[i].depthSlider.setSourceValue (sourceOutputs[i].load (std::memory_order_relaxed));
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (SECONDARY_COLOR);
    }

    void paintOverChildren (juce::Graphics& g) override
    {
        const int numRows = 12;
        const int rowHeight = getHeight() / numRows;
        g.setColour (TEXT_INACTIVE_COLOR.withAlpha (0.3f));
        for (int i = 1; i < numRows; ++i)
            g.drawHorizontalLine (i * rowHeight, 0.0f, static_cast<float> (getWidth()));
    }

    void resized() override
    {
        auto area = getLocalBounds();
        const int numRows = 12;
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
        if (index < 0 || index >= 12)
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

            const auto inner = bounds.reduced (5.0f);
            juce::Path icon;

            const float cx = inner.getCentreX();
            const float cy = inner.getCentreY();
            const float hw = inner.getWidth() * 0.45f;
            const float ah = inner.getHeight() * 0.25f;

            if (bipolar)
            {
                // Double-sided arrow <-->
                icon.startNewSubPath (cx - hw, cy);
                icon.lineTo (cx + hw, cy);
                // Left arrowhead
                icon.startNewSubPath (cx - hw + ah, cy - ah);
                icon.lineTo (cx - hw, cy);
                icon.lineTo (cx - hw + ah, cy + ah);
                // Right arrowhead
                icon.startNewSubPath (cx + hw - ah, cy - ah);
                icon.lineTo (cx + hw, cy);
                icon.lineTo (cx + hw - ah, cy + ah);
            }
            else
            {
                // Single right arrow -->
                icon.startNewSubPath (cx - hw, cy);
                icon.lineTo (cx + hw, cy);
                // Right arrowhead only
                icon.startNewSubPath (cx + hw - ah, cy - ah);
                icon.lineTo (cx + hw, cy);
                icon.lineTo (cx + hw - ah, cy + ah);
            }

            g.setColour (TEXT_COLOR);
            g.strokePath (icon, juce::PathStrokeType (1.5f));
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

            const auto inner = bounds.reduced (5.0f);
            const float cx = inner.getCentreX();
            const float cy = inner.getCentreY();
            const float r = juce::jmin (inner.getWidth(), inner.getHeight()) * 0.4f;

            const float alpha = bypassed ? 1.0f : 0.3f;
            g.setColour (TEXT_COLOR.withAlpha (alpha));

            // Circle
            g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 1.5f);

            // Diagonal line
            const float offset = r * 0.707f; // cos(45)
            g.drawLine (cx - offset, cy - offset, cx + offset, cy + offset, 1.5f);
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

            const auto inner = bounds.reduced (6.0f);
            const float alpha = isMouseOver() ? 0.7f : 0.3f;
            g.setColour (TEXT_COLOR.withAlpha (alpha));

            // X icon
            g.drawLine (inner.getX(), inner.getY(),
                        inner.getRight(), inner.getBottom(), 1.5f);
            g.drawLine (inner.getRight(), inner.getY(),
                        inner.getX(), inner.getBottom(), 1.5f);
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
    std::array<SlotRow, 12> rows;
};
