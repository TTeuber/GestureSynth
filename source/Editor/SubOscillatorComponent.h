//
// Created by Tyler Teuber on 5/10/25.
//

#pragma once

#include "Utility/DualParameterComponent.h"

class SubOscillatorComponent : public DualParameterComponent
{
public:
    SubOscillatorComponent (juce::RangedAudioParameter* subOscParam,
        juce::RangedAudioParameter* subOscWaveParam)
        : DualParameterComponent (subOscParam, subOscWaveParam)
    {
        // Ensure we're working with the correct parameters
        jassert (subOscParam->getName (15).contains ("Sub") && subOscWaveParam->getName (15).contains ("Wave"));
    }

    SubOscillatorComponent (juce::AudioProcessorValueTreeState& apvts,
        juce::UndoManager* undoManager = nullptr,
        std::atomic<int>* gestureCount = nullptr,
        ModulationModeState* modModeState = nullptr)
        : DualParameterComponent (
              apvts.getParameter ("subOsc"),
              apvts.getParameter ("subOscWave"),
              dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter ("subOn")),
              undoManager,
              gestureCount,
              modModeState)
    {
    }

    ~SubOscillatorComponent() override = default;

protected:
    void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const override
    {
        drawMorphedWaveform (g, bounds, param1Value, param2Value);
    }

    void drawVisualizationWithValues (juce::Graphics& g,
        const juce::Rectangle<int>& bounds, float p1, float p2) const override
    {
        drawMorphedWaveform (g, bounds, p1, p2);
    }

    juce::String getParam2Text() const override
    {
        const float v = param2Value;
        const float scaled = v * 3.0f;
        const int seg = juce::jmin (2, static_cast<int> (scaled));
        const float blend = scaled - static_cast<float> (seg);

        static const char* names[] = { "Sine", "Triangle", "Square", "Saw" };

        // At cardinal points, show just the wave name
        if (blend < 0.01f)
            return juce::String ("Wave: ") + names[seg];
        if (seg == 2 && blend > 0.99f)
            return "Wave: Saw";

        // Between cardinal points, show blend ratio
        int pctA = juce::roundToInt ((1.0f - blend) * 100.0f);
        int pctB = 100 - pctA;
        return juce::String (names[seg]) + " " + juce::String (pctA) + "/" + juce::String (pctB) + " " + names[seg + 1];
    }

private:
    // Returns waveform sample at position t (0-1) for a given wave index
    // 0=Sine, 1=Triangle, 2=Square, 3=Saw (down ramp)
    static float getWaveformSample (int waveIndex, float t)
    {
        switch (waveIndex)
        {
            case 0: // Sine
                return std::sin (t * juce::MathConstants<float>::twoPi);
            case 1: // Triangle
                return 2.0f * std::abs (2.0f * t - 1.0f) - 1.0f;
            case 2: // Square
                return (t < 0.5f) ? 1.0f : -1.0f;
            case 3: // Saw (down ramp)
                return 1.0f - 2.0f * t;
            default:
                return 0.0f;
        }
    }

    static void drawMorphedWaveform (juce::Graphics& g, const juce::Rectangle<int>& bounds,
        float amplitude, float waveParam)
    {
        const float width = static_cast<float> (bounds.getWidth());
        const float maxHeight = bounds.getHeight() / 2.0f * amplitude;
        const float centerY = static_cast<float> (bounds.getCentreY());
        const int numPoints = 128;

        const float scaled = waveParam * 3.0f;
        const int seg = juce::jmin (2, static_cast<int> (scaled));
        const float blend = scaled - static_cast<float> (seg);

        juce::Path wavePath;
        wavePath.startNewSubPath (static_cast<float> (bounds.getX()), centerY);

        for (int i = 0; i <= numPoints; ++i)
        {
            const float t = static_cast<float> (i) / static_cast<float> (numPoints);
            const float sA = getWaveformSample (seg, t);
            const float sB = getWaveformSample (seg + 1, t);
            const float sample = sA + blend * (sB - sA);

            const float x = static_cast<float> (bounds.getX()) + t * width;
            const float y = centerY - maxHeight * sample;
            wavePath.lineTo (x, y);
        }

        g.strokePath (wavePath, juce::PathStrokeType (2.0f));
    }
};
