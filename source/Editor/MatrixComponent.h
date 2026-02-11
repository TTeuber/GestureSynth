#pragma once

#include "../PluginProcessor.h"
#include "../Theme.h"
#include <juce_gui_basics/juce_gui_basics.h>

class MatrixComponent final : public juce::Component, public juce::ValueTree::Listener
{
public:
    explicit MatrixComponent (juce::ValueTree& mt) : modTree (mt)
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
            row.depthSlider.setRange (0.0, 1.0);
            row.depthSlider.setValue (child.getProperty ("depth"), juce::dontSendNotification);
            addAndMakeVisible (row.depthSlider);

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

            // Wire callbacks
            row.sourceBox.onChange = [this, i]()
            {
                auto selectedSource = PluginProcessor::modSourceIDs[rows[i].sourceBox.getSelectedId() - 1];
                modTree.getChild (i).setProperty ("source", selectedSource, nullptr);
                // Auto-set isBipolar based on source
                bool bipolar = selectedSource.startsWith ("lfo");
                modTree.getChild (i).setProperty ("isBipolar", bipolar, nullptr);
            };

            row.depthSlider.onValueChange = [this, i]()
            {
                modTree.getChild (i).setProperty ("depth", static_cast<float> (rows[i].depthSlider.getValue()), nullptr);
            };

            row.destBox.onChange = [this, i]()
            {
                auto selectedDest = PluginProcessor::modDestIDs[rows[i].destBox.getSelectedId() - 1];
                modTree.getChild (i).setProperty ("destination", selectedDest, nullptr);
            };
        }
    }

    ~MatrixComponent() override
    {
        modTree.removeListener (this);
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

            row.sourceBox.setBounds (rowArea.removeFromLeft (srcWidth));
            row.destBox.setBounds (rowArea.removeFromRight (dstWidth));
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
                static_cast<double> (treeWhosePropertyHasChanged.getProperty ("depth")),
                juce::dontSendNotification);
        }
    }

    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override {}
    void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override {}

private:
    juce::ValueTree& modTree;

    struct SlotRow
    {
        juce::ComboBox sourceBox;
        juce::Slider   depthSlider { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
        juce::ComboBox destBox;
    };
    std::array<SlotRow, 12> rows;
};
