//
// Created by Tyler Teuber
//

#pragma once

#include "Utility/DualParameterComponent.h"
#include "../Utility/Parameters.h"

class NoiseComponent final : public DualParameterComponent
{
public:
    NoiseComponent (const juce::AudioProcessorValueTreeState& apvts,
        const UIContext& ctx = {})
        : DualParameterComponent (
              apvts.getParameter (ParamIDs::noiseLevel),
              apvts.getParameter (ParamIDs::noiseTone),
              nullptr,
              ctx,
              {},
              {},
              "Noise")
    {
    }

protected:
    void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const override
    {
        drawNoisePath (g, bounds, param1Value, param2Value);
    }

    void drawVisualizationWithValues (juce::Graphics& g,
        const juce::Rectangle<int>& bounds, float p1, float p2) const override
    {
        drawNoisePath (g, bounds, p1, p2);
    }

    juce::String getParam1Text() const override
    {
        return juce::StringRef ("Level: ") + juce::String (juce::roundToInt (param1Value * 100.0f)) + juce::StringRef ("%");
    }

    juce::String getParam2Text() const override
    {
        return juce::StringRef ("Tone: ") + juce::String (juce::roundToInt (param2Value * 100.0f)) + juce::StringRef ("%");
    }

private:
    static void drawNoisePath (juce::Graphics& g, const juce::Rectangle<int>& bounds,
        float level, float tone)
    {
        constexpr float strokeThickness = 1.5f;
        constexpr int numVertices = 40;
        constexpr float maxTiltAngle = 15.0f;

        const float w = static_cast<float> (bounds.getWidth());
        const float h = static_cast<float> (bounds.getHeight());
        const float startX = static_cast<float> (bounds.getX());
        const float midY = static_cast<float> (bounds.getCentreY());
        const float centreX = startX + w * 0.5f;

        const float maxAmplitude = h * 0.40f;
        const float amplitude = maxAmplitude * level;

        // Tilt: tone=0 -> clockwise (positive slope), tone=0.5 -> flat, tone=1 -> counter-clockwise
        const float tiltNorm = (0.5f - tone) * 2.0f; // +1 at tone=0, 0 at tone=0.5, -1 at tone=1
        const float tiltRadians = tiltNorm * maxTiltAngle * juce::MathConstants<float>::pi / 180.0f;
        const float tiltSlope = std::tan (tiltRadians);

        juce::Random jaggedRng (42);
        juce::Path path;

        for (int v = 0; v <= numVertices; ++v)
        {
            const float frac = static_cast<float> (v) / static_cast<float> (numVertices);
            const float x = startX + frac * w;
            const float noise = jaggedRng.nextFloat() * 2.0f - 1.0f;
            const float tiltOffset = (x - centreX) * tiltSlope;
            const float y = midY + tiltOffset - noise * amplitude;

            if (v == 0)
                path.startNewSubPath (x, y);
            else
                path.lineTo (x, y);
        }

        g.strokePath (path, juce::PathStrokeType (strokeThickness));
    }
};
