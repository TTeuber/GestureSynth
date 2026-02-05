#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_dsp/juce_dsp.h>
#include <ranges>

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

    LFOPoint (const float pos, const float val, const float curveParam = 0.0f)
        : position (pos), value (val), curve (curveParam)
    {
    }
};

/**
 *
 * Class to hold LFO data that can be shared between audio and GUI threads
 */
class LFOData
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void lfoDataChanged() = 0;
    };
    void addListener (Listener* listener)
    {
        juce::SpinLock::ScopedLockType lock (listenersLock);
        listeners.addIfNotAlreadyThere (listener);
    }

    void removeListener (Listener* listener)
    {
        juce::SpinLock::ScopedLockType lock (listenersLock);
        listeners.removeFirstMatchingValue (listener);
    }

    void updateListeners()
    {
        // Copy listeners while holding lock, then notify without lock
        // to avoid deadlock if callback modifies LFO data
        juce::Array<Listener*> listenersCopy;
        {
            juce::SpinLock::ScopedLockType lock (listenersLock);
            listenersCopy = listeners;
        }
        for (Listener* listener : listenersCopy)
        {
            listener->lfoDataChanged();
        }
    }
    LFOData()
    {
        // Initialize with a default sine wave
        setShape (LFOShape::Sine);
    }

    /**
     * Sets the LFO shape to one of the predefined shapes
     */
    void setShape (const LFOShape shape)
    {
        juce::SpinLock::ScopedLockType lock (pointsLock);
        points.clear();

        switch (shape)
        {
            case LFOShape::Sine:
                // Points with curves to create a sine-like shape
                points = {
                    { 0.0f, 0.5f, -0.8f },
                    { 0.25f, 1.0f, 0.8f },
                    { 0.5f, 0.5f, -0.8f },
                    { 0.75f, 0.0f, 0.8f },
                    { 1.0f, 0.5f, 0.0f }
                };
                break;

            case LFOShape::Triangle:
                points = {
                    { 0.0f, 0.0f, 0.0f },
                    { 0.5f, 1.0f, 0.0f },
                    { 1.0f, 0.0f, 0.0f }
                };
                break;

            case LFOShape::Square:
                points = {
                    { 0.0f, 0.0f, 0.0f },
                    { 0.001f, 1.0f, 0.0f },
                    { 0.5f, 1.0f, 0.0f },
                    { 0.501f, 0.0f, 0.0f },
                    { 1.0f, 0.0f, 0.0f }
                };
                break;

            case LFOShape::RampUp:
                points = {
                    { 0.0f, 0.0f, 0.0f },
                    { 1.0f, 1.0f, 0.0f }
                };
                break;

            case LFOShape::RampDown:
                points = {
                    { 0.0f, 1.0f, 0.0f },
                    { 1.0f, 0.0f, 0.0f }
                };
                break;

            case LFOShape::Custom:
                // Do nothing, keep the existing points
                break;
        }

        sortPoints();
    }

    /**
     * Adds a new point to the LFO shape
     */
    void addPoint (float position, float value, float curve = 0.0f)
    {
        juce::SpinLock::ScopedLockType lock (pointsLock);

        // Ensure position and value are in valid range
        position = juce::jlimit (0.0f, 1.0f, position);
        value = juce::jlimit (0.0f, 1.0f, value);
        curve = juce::jlimit (-1.0f, 1.0f, curve);

        points.emplace_back (position, value, curve);
        sortPoints();
        updateListeners();
    }

    /**
     * Clears all points from the LFO shape
     */
    void clearPoints()
    {
        juce::SpinLock::ScopedLockType lock (pointsLock);
        points.clear();
        updateListeners();
    }

    /**
     * Sets an array of points to define the LFO shape
     */
    void setPoints (const std::vector<LFOPoint>& newPoints)
    {
        juce::SpinLock::ScopedLockType lock (pointsLock);
        points = newPoints;
        sortPoints();
        updateListeners();
    }

    /**
     * Gets a copy of the current array of points
     */
    std::vector<LFOPoint> getPoints() const
    {
        juce::SpinLock::ScopedLockType lock (pointsLock);
        return points; // Return a copy to ensure thread safety
    }

    /**
     * Gets a value at a specific normalized position in the waveform
     */
    float getValueAt (float normalizedPosition) const
    {
        juce::SpinLock::ScopedLockType lock (pointsLock);

        // If no points are defined, return 0
        if (points.empty())
            return 0.0f;

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
        return p1.value + curvedPosition * (p2.value - p1.value);
    }

private:
    /**
     * Sorts the points by position
     * Note: This should be called with the lock already acquired
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
    mutable juce::SpinLock pointsLock; // Lock for thread-safe access to points
    juce::Array<Listener*> listeners; // List of listeners to notify when the LFO data changes
    mutable juce::SpinLock listenersLock; // Lock for thread-safe access to listeners
};