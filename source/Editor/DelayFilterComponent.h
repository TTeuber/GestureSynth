//
// Delay Filter Visualization Components
//

#pragma once

#include "Utility/SingleParameterComponent.h"

class DelayHighpassComponent : public SingleParameterComponent
{
public:
    using SingleParameterComponent::SingleParameterComponent;

protected:
    void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const override
    {
        const float cutoffHz = param->convertFrom0to1 (paramValue);

        juce::Path path;
        constexpr int step = 3;
        constexpr double minFreq = 20.0;
        constexpr double maxFreq = 3000.0;
        constexpr double curveStartFreq = 5.0;

        const int startPixel = static_cast<int> (logFreqToX (curveStartFreq, minFreq, maxFreq) * bounds.getWidth()) + bounds.getX();
        const float centreY = bounds.getCentreY();
        bool started = false;

        for (int xPx = startPixel; xPx < bounds.getRight(); xPx += step)
        {
            const double x = static_cast<double> (xPx - bounds.getX()) / bounds.getWidth();
            const double freq = xToLogFreq (x, minFreq, maxFreq);
            const double dB = computeHPGainDb (freq, cutoffHz);
            const float yPx = centreY - static_cast<float> (dB) * bounds.getHeight() / 50.0f;

            if (! started)
            {
                path.startNewSubPath (static_cast<float> (xPx), yPx);
                started = true;
            }
            else
                path.lineTo (static_cast<float> (xPx), yPx);
        }

        g.setColour (TEXT_COLOR);
        g.strokePath (path, juce::PathStrokeType (2.0f));
    }

private:
    static double computeHPGainDb (double freq, double cutoffHz)
    {
        if (cutoffHz <= 0.0 || freq <= 0.0)
            return -60.0;

        const double w = freq / cutoffHz;
        const double wSq = w * w;
        const double q = 1.0 / std::sqrt (2.0);
        const double denom = std::sqrt ((1.0 - wSq) * (1.0 - wSq) + (w / q) * (w / q));

        if (denom < 1e-10)
            return -60.0;

        const double gain = wSq / denom;
        if (gain < 1e-10)
            return -60.0;

        return 2.0 * 20.0 * std::log10 (gain);
    }

    static double logFreqToX (double freq, double minF, double maxF)
    {
        if (freq <= 0.0) return 0.0;
        return std::log (freq / minF) / std::log (maxF / minF);
    }

    static double xToLogFreq (double x, double minF, double maxF)
    {
        return minF * std::pow (maxF / minF, x);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayHighpassComponent)
};

class DelayLowpassComponent : public SingleParameterComponent
{
public:
    using SingleParameterComponent::SingleParameterComponent;

protected:
    void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const override
    {
        const float cutoffHz = param->convertFrom0to1 (paramValue);

        juce::Path path;
        constexpr int step = 3;
        constexpr double minFreq = 200.0;
        constexpr double maxFreq = 20000.0;

        const float centreY = bounds.getCentreY();
        bool started = false;

        for (int xPx = bounds.getX(); xPx < bounds.getRight(); xPx += step)
        {
            const double x = static_cast<double> (xPx - bounds.getX()) / bounds.getWidth();
            const double freq = xToLogFreq (x, minFreq, maxFreq);
            const double dB = computeLPGainDb (freq, cutoffHz);
            const float yPx = centreY - static_cast<float> (dB) * bounds.getHeight() / 50.0f;

            if (! started)
            {
                path.startNewSubPath (static_cast<float> (xPx), yPx);
                started = true;
            }
            else
                path.lineTo (static_cast<float> (xPx), yPx);
        }

        g.setColour (TEXT_COLOR);
        g.strokePath (path, juce::PathStrokeType (2.0f));
    }

private:
    static double computeLPGainDb (double freq, double cutoffHz)
    {
        if (cutoffHz <= 0.0 || freq <= 0.0)
            return -60.0;

        const double w = freq / cutoffHz;
        const double wSq = w * w;
        const double q = 1.0 / std::sqrt (2.0);
        const double denom = std::sqrt ((1.0 - wSq) * (1.0 - wSq) + (w / q) * (w / q));

        if (denom < 1e-10)
            return -60.0;

        const double gain = 1.0 / denom;
        if (gain < 1e-10)
            return -60.0;

        return 2.0 * 20.0 * std::log10 (gain);
    }

    static double xToLogFreq (double x, double minF, double maxF)
    {
        return minF * std::pow (maxF / minF, x);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayLowpassComponent)
};
