#pragma once

#include "Modulation.h"
#include "Theme.h"
#include <juce_gui_basics/juce_gui_basics.h>

class MatrixComponent final : public juce::Component, public juce::ValueTree::Listener
{
public:
    using ModList = std::vector<std::tuple<juce::String, float, juce::String>>;
    explicit MatrixComponent (juce::ValueTree& mt) : modTree (mt)
    {
        modTree.addListener (this);
        for (int i = 0; i < modTree.getNumChildren(); i++)
        {
            auto* rotarySlider = new juce::Slider (juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox);
            rotarySlider->setValue (modTree.getChild (i).getProperty ("depth")); // Set initial value
            rotarySlider->setRange (0.0, 1.0); // Set range for the slider
            rotarySliders.push_back (rotarySlider); // Store in the vector
            addAndMakeVisible (rotarySlider); // Add to the interface
            rotarySlider->onValueChange = [this, i] {
                modTree.getChild (i).setProperty ("depth", rotarySliders[i]->getValue(), nullptr);
            };
        }
    }

    ~MatrixComponent() override
    {
        // Destroy dynamically allocated sliders
        for (const auto* rotarySlider : rotarySliders)
        {
            delete rotarySlider;
        }
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (SECONDARY_COLOR);

        g.setColour (TEXT_COLOR);
        juce::Rectangle<int> area = getLocalBounds();
        const auto rowHeight = area.getHeight() / modTree.getNumChildren();
        size_t sliderIndex = 0;

        for (int i = 0; i < modTree.getNumChildren(); i++)
        {
            const juce::StringRef source = modTree.getChild (i).getProperty ("source").toString();
            const juce::StringRef destination = modTree.getChild (i).getProperty ("destination").toString();
            auto row = area.removeFromTop (rowHeight);
            const auto colWidth = row.getWidth() / 3;

            g.drawText (source, row.removeFromLeft (colWidth), juce::Justification::centred, true);

            // Set the bounds of the existing slider from the vector
            if (sliderIndex < rotarySliders.size())
            {
                rotarySliders[sliderIndex++]->setBounds (row.removeFromLeft (colWidth));
            }

            g.drawText (destination, row.removeFromLeft (colWidth), juce::Justification::centred, true);
        }
    }

    void resized() override
    {
        // For good measure, update the bounds here if necessary
        auto area = getLocalBounds();
        const auto rowHeight = static_cast<int> (area.getHeight() / modTree.getNumChildren());
        size_t sliderIndex = 0;

        for (size_t i = 0; i < modTree.getNumChildren(); ++i)
        {
            auto row = area.removeFromTop (rowHeight);
            const auto colWidth = row.getWidth() / 3;

            row.removeFromLeft (colWidth); // Skip the first column for source text

            if (sliderIndex < rotarySliders.size())
            {
                rotarySliders[sliderIndex++]->setBounds (row.removeFromLeft (colWidth));
            }
        }
    }

private:
    juce::ValueTree& modTree;

    std::vector<juce::Slider*> rotarySliders; // Vector to store slider pointers
};