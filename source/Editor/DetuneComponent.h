//
// Created by Tyler Teuber on 5/9/25.
//

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Utility/DualParameterComponent.h"

class DetuneComponent final : public DualParameterComponent, private juce::Timer
{
public:
    DetuneComponent (juce::RangedAudioParameter* detuneParam,
        juce::RangedAudioParameter* stereoParam,
        juce::AudioParameterBool* stereoParamBool = nullptr)
        : DualParameterComponent (detuneParam, stereoParam, stereoParamBool)
    {
    }

    DetuneComponent (juce::AudioProcessorValueTreeState& apvts,
        juce::UndoManager* undoManager = nullptr,
        std::atomic<int>* gestureCount = nullptr,
        std::atomic<float>* modDetuneOutput = nullptr,
        std::atomic<float>* modWidthOutput = nullptr,
        ModulationModeState* modModeState = nullptr,
        const juce::String& param1DestID = {},
        const juce::String& param2DestID = {})
        : DualParameterComponent (
              apvts.getParameter ("oscDetune"),
              apvts.getParameter ("oscWidth"),
              dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter ("detuneOn")),
              undoManager,
              gestureCount,
              modModeState,
              param1DestID,
              param2DestID),
          modDetuneOutput (modDetuneOutput),
          modWidthOutput (modWidthOutput)
    {
        if (modDetuneOutput != nullptr || modWidthOutput != nullptr)
            startTimerHz (30);
    }

    ~DetuneComponent() override { stopTimer(); }

protected:
    void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const override
    {
        // Draw ghost path at modulated values (behind main)
        if ((modDetuneOutput != nullptr || modWidthOutput != nullptr)
            && (std::abs (modulatedDetune - param1Value) > 0.001f
                || std::abs (modulatedWidth - param2Value) > 0.001f))
        {
            g.setColour (TEXT_COLOR.withAlpha (0.25f));
            drawPathWithValues (g, bounds, modulatedDetune, modulatedWidth);
        }

        g.setColour (TEXT_COLOR);
        drawPathWithValues (g, bounds, param1Value, param2Value);
    }

    void drawVisualizationWithValues (juce::Graphics& g,
        const juce::Rectangle<int>& bounds, float p1, float p2) const override
    {
        drawPathWithValues (g, bounds, p1, p2);
    }

private:
    std::atomic<float>* modDetuneOutput = nullptr;
    std::atomic<float>* modWidthOutput = nullptr;
    float modulatedDetune = 0.0f;
    float modulatedWidth = 0.0f;

    void timerCallback() override
    {
        constexpr float alpha = 0.3f;
        if (modDetuneOutput != nullptr)
        {
            float target = modDetuneOutput->load (std::memory_order_relaxed);
            modulatedDetune += alpha * (target - modulatedDetune);
        }
        if (modWidthOutput != nullptr)
        {
            float target = modWidthOutput->load (std::memory_order_relaxed);
            modulatedWidth += alpha * (target - modulatedWidth);
        }
        repaint();
    }

    void drawPathWithValues (juce::Graphics& g, const juce::Rectangle<int>& bounds,
        float p1, float p2) const
    {
        // Create path
        juce::Path path;
        const float width = bounds.getWidth();

        auto pointsL = generateWavyCircle (150, 1.0, 8, p1 * 0.5);
        auto started = false;
        for (auto& [x, y] : pointsL)
        {
            if (!started)
            {
                path.startNewSubPath (x + bounds.getCentreX() + p2 * width * 0.2, y + bounds.getCentreY());
                started = true;
            }
            else
                path.lineTo (x + bounds.getCentreX() + p2 * width * 0.2, y + bounds.getCentreY());
        }
        path.closeSubPath();

        // Draw the path
        g.strokePath (path, juce::PathStrokeType (2.0f));

        auto pointsR = generateWavyCircle (150, 1.0, 8, -p1 * 0.5);
        started = false;
        for (auto& [x, y] : pointsR)
        {
            if (!started)
            {
                path.startNewSubPath (x + bounds.getCentreX() - p2 * width * 0.2, y + bounds.getCentreY());
                started = true;
            }
            else
                path.lineTo (x + bounds.getCentreX() - p2 * width * 0.2, y + bounds.getCentreY());
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
};