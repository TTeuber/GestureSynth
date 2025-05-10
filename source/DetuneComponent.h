//
// Created by Tyler Teuber on 5/9/25.
//

#pragma once

#include "Theme.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

class DetuneComponent final : public juce::Component,
                              private juce::AudioProcessorValueTreeState::Listener
{
public:
    DetuneComponent (juce::AudioProcessorValueTreeState& apvts,
        const juce::StringRef detuneParamId,
        const juce::StringRef stereoParamId)
        : apvts (apvts),
          detuneParamId (detuneParamId),
          stereoParamId (stereoParamId)
    {
        // Register as a listener for both parameters
        apvts.addParameterListener (detuneParamId, this);
        apvts.addParameterListener (stereoParamId, this);

        // Set initial values from parameters
        detuneValue = apvts.getParameter (detuneParamId)->getValue();
        stereoValue = apvts.getParameter (stereoParamId)->getValue();
    }

    ~DetuneComponent() override
    {
        // Remove listeners
        apvts.removeParameterListener (detuneParamId, this);
        apvts.removeParameterListener (stereoParamId, this);
    }

    void paint (juce::Graphics& g) override
    {
        // Draw background
        g.fillAll (SECONDARY_COLOR);

        // Calculate dimensions for the waveform
        const auto bounds = getLocalBounds().reduced (20);
        // const float centerY = bounds.getCentreY();
        // const float centerX = bounds.getCentreX();

        // Draw the main waveform
        g.setColour (TEXT_COLOR);
        drawPath (g, bounds);

        // Draw percentage text indicators
        g.setColour (juce::Colours::white);
        g.setFont (14.0f);

        // Display detune percentage (bottom left)
        int detunePercentage = static_cast<int> (detuneValue * 100.0f);
        g.drawText ("detune: " + juce::String (detunePercentage) + "%",
            10,
            getHeight() - 25,
            120,
            20,
            juce::Justification::bottomLeft,
            true);

        // Display pulse width percentage (top right)
        int stereoPercentage = static_cast<int> (stereoValue * 100.0f);
        g.drawText ("stereo: " + juce::String (stereoPercentage) + "%",
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
        auto size = juce::jmin (getWidth(), getHeight());
        setBounds (getX(), getY(), size, size);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        // Store initial mouse position and parameter values for drag
        mouseDownX = e.x;
        mouseDownY = e.y;
        initialDetuneValue = detuneValue;
        initialStereoValue = stereoValue;

        isDragging = true;
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (!isDragging)
            return;

        const auto bounds = getLocalBounds();

        // Calculate vertical movement for waveform parameter (up = more square, down = more saw)
        const float verticalDelta = (mouseDownY - e.y) / static_cast<float> (bounds.getHeight() - padding);
        const float newDetuneValue = juce::jlimit (0.0f, 1.0f, initialDetuneValue + verticalDelta);

        // Calculate horizontal movement for pulse width parameter (right = more width)
        const float horizontalDelta = (e.x - mouseDownX) / static_cast<float> (bounds.getWidth() - padding);
        const float newStereoValue = juce::jlimit (0.0f, 1.0f, initialStereoValue + horizontalDelta);

        // Update parameters in the AudioProcessorValueTreeState
        apvts.getParameter (detuneParamId)->setValueNotifyingHost (newDetuneValue);
        apvts.getParameter (stereoParamId)->setValueNotifyingHost (newStereoValue);
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        isDragging = false;
    }

private:
    void parameterChanged (const juce::String& parameterID, const float newValue) override
    {
        // Update local values when parameters change
        if (parameterID == detuneParamId)
            detuneValue = apvts.getParameterRange (detuneParamId).convertTo0to1 (newValue);
        else if (parameterID == stereoParamId)
            stereoValue = apvts.getParameterRange (stereoParamId).convertTo0to1 (newValue);

        repaint();
    }

    void drawPath (juce::Graphics& g, const juce::Rectangle<int>& bounds) const
    {
        // Create path
        juce::Path path;
        const float width = bounds.getWidth();
        const float height = bounds.getHeight();

        auto pointsL = generateWavyCircle (150, 1.0, 8, detuneValue * 0.5);
        auto started = false;
        for (auto& [x, y] : pointsL)
        {
            if (!started)
            {
                path.startNewSubPath (x + bounds.getCentreX() + stereoValue * width * 0.2, y + bounds.getCentreY());
                started = true;
            }
            else
                path.lineTo (x + bounds.getCentreX() + stereoValue * width * 0.2, y + bounds.getCentreY());
        }
        path.closeSubPath();

        // Draw the path
        g.strokePath (path, juce::PathStrokeType (2.0f));

        auto pointsR = generateWavyCircle (150, 1.0, 8, -detuneValue * 0.5);
        started = false;
        for (auto& [x, y] : pointsR)
        {
            if (!started)
            {
                path.startNewSubPath (x + bounds.getCentreX() - stereoValue * width * 0.2, y + bounds.getCentreY());
                started = true;
            }
            else
                path.lineTo (x + bounds.getCentreX() - stereoValue * width * 0.2, y + bounds.getCentreY());
        }
        path.closeSubPath();

        // Draw the path
        g.strokePath (path, juce::PathStrokeType (2.0f));
    }

    /**
     * Calculate coordinates for a wavy circle with balanced inner and outer curvature.
     *
     * @param theta Angle in radians
     * @param baseRadius The average radius of the circle
     * @param waveCount Number of waves around the circle
     * @param waveDepth Amplitude of the waves (relative to base_radius)
     * @return std::pair<double, double> The (x, y) coordinates
     */
    std::pair<double, double> wavyCircle (double theta,
        double baseRadius = 1.0,
        int waveCount = 8,
        double waveDepth = 0.2) const
    {
        double polarity = 1.0;
        if (waveDepth < 0.0)
        {
            waveDepth = -waveDepth;
            polarity = -1.0;
        }
        // Calculate the modified radius using the improved formula
        const double sinTerm = std::sin (waveCount * theta);
        const double absSinTerm = std::abs (sinTerm);

        // This creates waves with more consistent curvature
        const double r = baseRadius * (1.0 + waveDepth * polarity * sinTerm / (1.0 + waveDepth * absSinTerm));

        // Convert from polar to Cartesian coordinates
        double x = r * std::cos (theta) * getWidth() / 2 * 0.4;
        double y = r * std::sin (theta) * getHeight() / 2 * 0.4;

        return std::make_pair (x, y);
    }

    /**
     * Generate multiple points for a wavy circle.
     *
     * @param numPoints Number of points to generate around the circle
     * @param baseRadius The average radius of the circle
     * @param waveCount Number of waves around the circle
     * @param waveDepth Amplitude of the waves (relative to base_radius)
     * @return std::vector<std::pair<double, double>> Vector of (x, y) coordinates
     */
    std::vector<std::pair<double, double>> generateWavyCircle (int numPoints = 100,
        double baseRadius = 1.0,
        int waveCount = 8,
        double waveDepth = 0.2) const
    {
        std::vector<std::pair<double, double>> points;
        points.reserve (numPoints);

        // Generate points evenly spaced around the circle
        for (int i = 0; i < numPoints; ++i)
        {
            const double theta = juce::MathConstants<double>::twoPi * i / numPoints;
            points.push_back (wavyCircle (theta, baseRadius, waveCount, waveDepth));
        }

        return points;
    }

    // Reference to the value tree
    juce::AudioProcessorValueTreeState& apvts;

    // Parameter IDs
    juce::String detuneParamId;
    juce::String stereoParamId;

    // Current parameter values
    float detuneValue = 0.0f;
    float stereoValue = 0.0f;

    float padding = 0.4f; // Padding for waveform drawing

    // Mouse drag tracking
    bool isDragging = false;
    int mouseDownX = 0;
    int mouseDownY = 0;
    float initialDetuneValue = 0.0f;
    float initialStereoValue = 0.0f;
};
