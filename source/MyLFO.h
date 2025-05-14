//
// Created by Tyler Teuber on 3/22/25.
//
#pragma once

#include "Modulation.h"
#include <algorithm>
#include <functional>
#include <juce_dsp/juce_dsp.h>
#include <vector>

/**
 * Enumeration for predefined LFO shapes
 */
enum class LFOShape {
    Sine,
    Triangle,
    Square,
    RampUp,
    RampDown,
    Custom
};

/**
 * Structure to represent a point in the LFO waveform
 * position: normalized time position (0.0 to 1.0)
 * value: normalized output value (0.0 to 1.0)
 */
struct LFOPoint
{
    float position = 0.0f; // Time position (0.0 to 1.0)
    float value = 0.0f; // Output value (0.0 to 1.0)
    float curve = 0.0f; // Curve parameter (-1.0 to 1.0)
        // Negative: logarithmic, 0: linear, Positive: exponential

    LFOPoint (float pos, float val, const float curveParam = 0.0f)
        : position (pos), value (val), curve (curveParam)
    {
    }
};

/**
 * LFO class implementing the ModSource interface
 */
class MyLFO final : public ModSource
{
public:
    explicit MyLFO (const juce::String& id = "lfo", const juce::String& name = "LFO", const float rate = 1.0f)
        : ModSource (id, name),
          rate (rate),
          phase (0.0f),
          currentValue (0.0f),
          bipolar (false)
    {
        // Initialize with a sine wave by default
        setShape (LFOShape::Sine);
    }

    /**
     * Sets the LFO rate in Hz
     */
    void setRate (const float newRate) noexcept
    {
        rate = juce::jlimit (0.01f, 100.0f, newRate);
        updatePhaseIncrement();
    }

    /**
     * Gets the current LFO rate in Hz
     */
    [[nodiscard]] float getRate() const noexcept { return rate; }

    /**
     * Sets the LFO to bipolar mode (-1.0 to 1.0) or unipolar mode (0.0 to 1.0)
     */
    void setBipolar (const bool shouldBeBipolar) noexcept { bipolar = shouldBeBipolar; }

    /**
     * Returns whether the LFO is in bipolar mode
     */
    bool isBipolar() const noexcept { return bipolar; }

    /**
     * Sets the LFO shape to one of the predefined shapes
     */
    void setShape (const LFOShape shape)
    {
        points.clear();

        switch (shape)
        {
            case LFOShape::Sine:
                // Three points with curves to create a sine-like shape
                points = {
                    { 0.0f, 0.5f, 0.0f },
                    { 0.25f, 1.0f, -0.8f },
                    { 0.5f, 0.5f, 0.0f },
                    { 0.75f, 0.0f, -0.8f },
                    { 1.0f, 0.5f, 0.0f }
                };
                break;

            case LFOShape::Triangle:
                // Linear transitions between points
                points = {
                    { 0.0f, 0.0f, 0.0f },
                    { 0.5f, 1.0f, 0.0f },
                    { 1.0f, 0.0f, 0.0f }
                };
                break;

            case LFOShape::Square:
                // Sharp transitions between high and low
                points = {
                    { 0.0f, 0.0f, 0.0f },
                    { 0.001f, 1.0f, 0.0f },
                    { 0.5f, 1.0f, 0.0f },
                    { 0.501f, 0.0f, 0.0f },
                    { 1.0f, 0.0f, 0.0f }
                };
                break;

            case LFOShape::RampUp:
                // Linear ramp up
                points = {
                    { 0.0f, 0.0f, 0.0f },
                    { 1.0f, 1.0f, 0.0f }
                };
                break;

            case LFOShape::RampDown:
                // Linear ramp down
                points = {
                    { 0.0f, 1.0f, 0.0f },
                    { 1.0f, 0.0f, 0.0f }
                };
                break;

            case LFOShape::Custom:
                // Do nothing, keep the existing points
                break;
        }

        // Ensure the waveform is properly sorted
        sortPoints();
    }

    /**
     * Adds a new point to the LFO shape
     */
    void addPoint (float position, float value, float curve = 0.0f)
    {
        // Ensure position and value are in valid range
        position = juce::jlimit (0.0f, 1.0f, position);
        value = juce::jlimit (0.0f, 1.0f, value);
        curve = juce::jlimit (-1.0f, 1.0f, curve);

        points.emplace_back (position, value, curve);
        sortPoints();
    }

    /**
     * Clears all points from the LFO shape
     */
    void clearPoints()
    {
        points.clear();
    }

    /**
     * Sets an array of points to define the LFO shape
     */
    void setPoints (const std::vector<LFOPoint>& newPoints)
    {
        points = newPoints;
        sortPoints();
    }

    /**
     * Gets the current array of points
     */
    [[nodiscard]] const std::vector<LFOPoint>& getPoints() const noexcept
    {
        return points;
    }

    /**
     * Gets the current value of the LFO
     */
    [[nodiscard]] float getCurrentValue() const noexcept override
    {
        return currentValue;
    }

    /**
     * Generates the next value in the LFO sequence
     */
    float getNextValue() noexcept override
    {
        // If no points are defined, return 0
        if (points.empty())
        {
            currentValue = 0.0f;
            return currentValue;
        }

        // Calculate the current position in the waveform (0.0 to 1.0)
        const float normalizedPosition = phase;

        // Find the two points that surround the current position
        size_t index1 = 0;
        size_t index2 = 0;

        for (size_t i = 0; i < points.size() - 1; ++i)
        {
            if (normalizedPosition >= points[i].position && normalizedPosition <= points[i + 1].position)
            {
                index1 = i;
                index2 = i + 1;
                break;
            }
        }

        // Get the two surrounding points
        const LFOPoint& p1 = points[index1];
        const LFOPoint& p2 = points[index2];

        // Calculate the normalized position between the two points (0.0 to 1.0)
        const float segmentLength = p2.position - p1.position;
        const float positionInSegment = (normalizedPosition - p1.position) / (segmentLength > 0.0f ? segmentLength : 1.0f);

        // Apply curve to the interpolation
        const float curvedPosition = applyCurve (positionInSegment, p1.curve);

        // Linearly interpolate between the two points
        float output = p1.value + curvedPosition * (p2.value - p1.value);

        // Scale the output if bipolar
        if (bipolar)
            output = output * 2.0f - 1.0f;

        // Update the phase for the next sample
        phase += phaseIncrement;
        if (phase >= 1.0f)
            phase -= 1.0f;

        currentValue = output;
        return currentValue;
    }

    /**
     * Prepares the LFO for processing
     */
    void prepare (const juce::dsp::ProcessSpec& spec) override
    {
        ModSource::prepare (spec);
        updatePhaseIncrement();
    }

    /**
     * Resets the LFO phase to 0
     */
    void reset() noexcept
    {
        phase = 0.0f;
    }

    /**
     * Sets the LFO phase (0.0 to 1.0)
     */
    void setPhase (const float newPhase) noexcept
    {
        phase = juce::jlimit (0.0f, 1.0f, newPhase);
    }

private:
    /**
     * Updates the phase increment based on the current rate and sample rate
     */
    void updatePhaseIncrement() noexcept
    {
        phaseIncrement = rate / sampleRate;
    }

    /**
     * Sorts the points by position
     */
    void sortPoints()
    {
        std::ranges::sort (points, [] (const LFOPoint& a, const LFOPoint& b) {
            return a.position < b.position;
        });

        // Ensure we have at least two points
        if (points.empty())
        {
            // Default to a simple sine-like wave if no points are defined
            points = {
                { 0.0f, 0.5f, 0.0f },
                { 0.25f, 1.0f, -0.8f },
                { 0.5f, 0.5f, 0.0f },
                { 0.75f, 0.0f, -0.8f },
                { 1.0f, 0.5f, 0.0f }
            };
        }
        else if (points.size() == 1)
        {
            // If we only have one point, add another at the end of the cycle
            points.push_back ({ 1.0f, points[0].value, 0.0f });
        }

        // Ensure the first point is at position 0.0 and the last point is at position 1.0
        if (points.front().position > 0.0f)
        {
            points.insert (points.begin(), { 0.0f, points.front().value, 0.0f });
        }

        if (points.back().position < 1.0f)
        {
            points.push_back ({ 1.0f, points.front().value, 0.0f });
        }
    }

    /**
     * Applies a curve to a linear position
     * curve = -1.0: logarithmic curve
     * curve = 0.0: linear (no curve)
     * curve = 1.0: exponential curve
     */
    static float applyCurve (float position, const float curve) noexcept
    {
        // Ensure the position is in the valid range
        position = juce::jlimit (0.0f, 1.0f, position);

        if (std::abs (curve) < 0.001f)
            return position; // Linear

        // Apply an exponential or logarithmic curve
        float skew = std::powf (2.0f, std::abs (curve) * 5.0f);

        if (curve > 0.0f)
        {
            // Exponential
            return (std::powf (skew, position) - 1.0f) / (skew - 1.0f);
        }
        // Logarithmic
        return 1.0f - (std::powf (skew, 1.0f - position) - 1.0f) / (skew - 1.0f);
    }

    std::vector<LFOPoint> points; // Array of points defining the LFO shape
    float rate; // LFO rate in Hz
    float phase; // Current phase (0.0 to 1.0)
    float phaseIncrement; // Phase increment per sample
    float currentValue; // Current output value
    bool bipolar; // Whether the output is bipolar (-1.0 to 1.0) or unipolar (0.0 to 1.0)
};