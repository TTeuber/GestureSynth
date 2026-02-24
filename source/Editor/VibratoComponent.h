//
// Created by Tyler Teuber on 2/10/26.
//

#pragma once

#include "Utility/DualParameterComponent.h"

class VibratoComponent final : public DualParameterComponent
{
public:
    VibratoComponent (juce::RangedAudioParameter* depthParam,
        juce::RangedAudioParameter* rateParam,
        juce::AudioParameterBool* activeParam = nullptr)
        : DualParameterComponent (depthParam, rateParam, activeParam)
    {
    }

    explicit VibratoComponent (const juce::AudioProcessorValueTreeState& apvts,
        juce::UndoManager* undoManager = nullptr,
        std::atomic<int>* gestureCount = nullptr,
        ModulationModeState* modModeState = nullptr)
        : DualParameterComponent (
              apvts.getParameter ("vibratoDepth"),
              apvts.getParameter ("vibratoRate"),
              dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter ("vibratoOn")),
              undoManager,
              gestureCount,
              modModeState)
    {
    }

    ~VibratoComponent() override = default;

protected:
    void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const override
    {
        g.setColour (TEXT_COLOR);
        g.setOpacity (isActive ? 1.0f : 0.5f);

        constexpr float strokeThickness = 1.5f;

        const float depth = param1Value;
        const float rate = param2Value;

        const float rateFactor = 0.25f + rate * 0.5f;
        const float normFactor = 0.75f / (1.0f + depth);

        juce::Path path;
        const float midY = static_cast<float> (bounds.getCentreY());
        const float amplitude = bounds.getHeight() * 0.40f;
        const float width = static_cast<float> (bounds.getWidth());

        for (float px = 0; px <= width; px += 2.0f)
        {
            const float x = (px / width) * juce::MathConstants<float>::twoPi * 3.0f;
            const float yNorm = (std::sin (x * rateFactor) + depth * std::sin (10.0f * x * rateFactor)) * normFactor;
            const float y = midY - amplitude * yNorm;

            if (px == 0.0f)
                path.startNewSubPath (static_cast<float> (bounds.getX()) + px, y);
            else
                path.lineTo (static_cast<float> (bounds.getX()) + px, y);
        }

        g.strokePath (path, juce::PathStrokeType (strokeThickness));
    }

    juce::String getParam1Text() const override
    {
        const float depthSemitones = param1->convertFrom0to1 (param1Value);
        const float depthCents = depthSemitones * 100.0f;
        return juce::StringRef ("Depth: ") + juce::String (depthCents, depthCents >= 100.0f ? 0 : 1) + juce::StringRef (" cents");
    }

    juce::String getParam2Text() const override
    {
        const float rateHz = param2->convertFrom0to1 (param2Value);
        return juce::StringRef ("Rate: ") + juce::String (rateHz, rateHz >= 10.0f ? 1 : 2) + juce::StringRef (" Hz");
    }
};
