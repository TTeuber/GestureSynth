#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_dsp/juce_dsp.h>
#include <ranges>
#include "../Utility/CurveUtils.h"

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
                    { 0.0f, 0.5f, 0.13f },
                    { 0.25f, 1.0f, -0.13f },
                    { 0.5f, 0.5f, 0.13f },
                    { 0.75f, 0.0f, -0.13f },
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
                // First and last points must share a level for a seamless loop,
                // so the reset back to 0 happens as a steep drop at the very end.
                points = {
                    { 0.0f, 0.0f, 0.0f },
                    { 0.999f, 1.0f, 0.0f },
                    { 1.0f, 0.0f, 0.0f }
                };
                break;

            case LFOShape::RampDown:
                // First and last points must share a level for a seamless loop,
                // so the reset back to 1 happens as a steep rise at the very end.
                points = {
                    { 0.0f, 1.0f, 0.0f },
                    { 0.999f, 0.0f, 0.0f },
                    { 1.0f, 1.0f, 0.0f }
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
     * Returns the number of points without copying the vector
     */
    size_t getNumPoints() const
    {
        juce::SpinLock::ScopedLockType lock (pointsLock);
        return points.size();
    }

    /**
     * Updates a single point's position, value, and curve. Re-sorts and notifies listeners.
     */
    void updatePoint (size_t index, float position, float value, float curve)
    {
        juce::SpinLock::ScopedLockType lock (pointsLock);
        if (index >= points.size())
            return;

        points[index].position = juce::jlimit (0.0f, 1.0f, position);
        points[index].value = juce::jlimit (0.0f, 1.0f, value);
        points[index].curve = juce::jlimit (-1.0f, 1.0f, curve);
        sortPoints();
        updateListeners();
    }

    /**
     * Updates only the value of a point (useful for boundary linking)
     */
    void updatePointValue (size_t index, float value)
    {
        juce::SpinLock::ScopedLockType lock (pointsLock);
        if (index >= points.size())
            return;

        points[index].value = juce::jlimit (0.0f, 1.0f, value);
        updateListeners();
    }

    /**
     * Updates just the curve parameter of a point (no sort needed)
     */
    void updateCurve (size_t index, float curve)
    {
        juce::SpinLock::ScopedLockType lock (pointsLock);
        if (index >= points.size())
            return;

        points[index].curve = juce::jlimit (-1.0f, 1.0f, curve);
        updateListeners();
    }

    /**
     * Removes a point by index. Cannot remove first or last point, or if only 2 points remain.
     * Returns true if the point was removed.
     */
    bool removePoint (size_t index)
    {
        juce::SpinLock::ScopedLockType lock (pointsLock);
        if (points.size() <= 2 || index == 0 || index >= points.size() - 1)
            return false;

        points.erase (points.begin() + static_cast<std::ptrdiff_t> (index));
        updateListeners();
        return true;
    }

    /**
     * Serializes points to a ValueTree for state persistence
     */
    juce::ValueTree toValueTree() const
    {
        juce::SpinLock::ScopedLockType lock (pointsLock);
        juce::ValueTree tree ("LFOData");

        for (size_t i = 0; i < points.size(); ++i)
        {
            juce::ValueTree pointTree ("Point");
            pointTree.setProperty ("position", points[i].position, nullptr);
            pointTree.setProperty ("value", points[i].value, nullptr);
            pointTree.setProperty ("curve", points[i].curve, nullptr);
            tree.addChild (pointTree, -1, nullptr);
        }

        return tree;
    }

    /**
     * Deserializes points from a ValueTree
     */
    void fromValueTree (const juce::ValueTree& tree)
    {
        if (!tree.isValid() || tree.getType().toString() != "LFOData")
            return;

        std::vector<LFOPoint> newPoints;
        for (int i = 0; i < tree.getNumChildren(); ++i)
        {
            auto pointTree = tree.getChild (i);
            if (pointTree.getType().toString() == "Point")
            {
                float pos = pointTree.getProperty ("position", 0.0f);
                float val = pointTree.getProperty ("value", 0.0f);
                float crv = pointTree.getProperty ("curve", 0.0f);
                newPoints.emplace_back (pos, val, crv);
            }
        }

        if (!newPoints.empty())
        {
            juce::SpinLock::ScopedLockType lock (pointsLock);
            points = newPoints;
            sortPoints();
            updateListeners();
        }
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
                { 0.0f, 0.5f, -0.13f },
                { 0.25f, 1.0f, 0.13f },
                { 0.5f, 0.5f, -0.13f },
                { 0.75f, 0.0f, 0.13f },
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
        return CurveUtils::applyCurve (juce::jlimit (0.0f, 1.0f, position), curve);
    }

    std::vector<LFOPoint> points; // Array of points defining the LFO shape
    mutable juce::SpinLock pointsLock; // Lock for thread-safe access to points
    juce::Array<Listener*> listeners; // List of listeners to notify when the LFO data changes
    mutable juce::SpinLock listenersLock; // Lock for thread-safe access to listeners
};