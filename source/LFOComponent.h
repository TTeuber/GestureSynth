#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "LFOData.h"

class LFOComponent final : public juce::Component, LFOData::Listener
{
public:
    explicit LFOComponent (std::shared_ptr<LFOData> lfoData) : lfoData (std::move (lfoData))
    {
        this->lfoData->addListener (this);
        // Set default size
        setSize (300, 200);
    }

    void lfoDataChanged() override
    {
        DBG ("Changed");
        repaint();
    };

    void setLFOData (const std::shared_ptr<LFOData>& data)
    {
        lfoData = data;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        if (!lfoData)
            return;

        // Draw background
        g.fillAll (juce::Colours::black);

        // Get a copy of the current points for thread-safe access
        const auto points = lfoData->getPoints();
        if (points.empty())
            return;

        // Set up drawing parameters
        const auto width = getWidth();
        const auto height = getHeight();
        constexpr float margin = 10.0f;
        const float graphWidth = width - 2.0f * margin;
        const float graphHeight = height - 2.0f * margin;
        const float centerY = height / 2.0f;

        // Draw the center line
        g.setColour (juce::Colours::grey.withAlpha (0.5f));
        g.drawLine (margin, centerY, width - margin, centerY, 1.0f);

        // Draw the LFO waveform
        g.setColour (juce::Colours::white);

        // Generate a path for the waveform
        juce::Path path;
        bool pathStarted = false;

        // Draw points at a higher resolution than just the control points
        for (float t = 0.0f; t <= 1.0f; t += 0.005f)
        {
            const float value = lfoData->getValueAt (t);
            const float x = margin + t * graphWidth;
            const float y = margin + (1.0f - value) * graphHeight;

            if (!pathStarted)
            {
                path.startNewSubPath (x, y);
                pathStarted = true;
            }
            else
            {
                path.lineTo (x, y);
            }
        }

        g.strokePath (path, juce::PathStrokeType (2.0f));

        // Draw control points
        g.setColour (juce::Colours::orange);
        for (const auto& point : points)
        {
            const float x = margin + point.position * graphWidth;
            const float y = margin + (1.0f - point.value) * graphHeight;
            g.fillEllipse (x - 4.0f, y - 4.0f, 8.0f, 8.0f);
        }
    }

    // Add mouse interaction for editing points...
    void mouseDown (const juce::MouseEvent& e) override
    {
        // Implementation for adding/selecting points
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        // Implementation for moving points
    }

private:
    std::shared_ptr<LFOData> lfoData;
    // Other UI state variables...
};