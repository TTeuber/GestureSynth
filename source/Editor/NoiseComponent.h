//
// Created by Tyler Teuber
//

#pragma once

#include "Utility/AnimationFrameSource.h"
#include "Utility/DualParameterComponent.h"
#include "../Utility/AtomicHelpers.h"
#include "../Utility/Parameters.h"

class NoiseComponent final : public DualParameterComponent, public AnimationFrameSource::Listener
{
public:
    NoiseComponent (const juce::AudioProcessorValueTreeState& apvts,
        const UIContext& ctx = {},
        std::atomic<float>* modLevelOutputToUse = nullptr,
        std::atomic<float>* modToneOutputToUse = nullptr,
        const juce::String& param1DestIDToUse = {},
        const juce::String& param2DestIDToUse = {})
        : DualParameterComponent (
              apvts.getParameter (ParamIDs::noiseLevel),
              apvts.getParameter (ParamIDs::noiseTone),
              dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (ParamIDs::noiseOn)),
              ctx,
              param1DestIDToUse,
              param2DestIDToUse,
              "Noise"),
          modLevelOutput (modLevelOutputToUse),
          modToneOutput (modToneOutputToUse),
          animSource (ctx.animationSource)
    {
        if ((modLevelOutput != nullptr || modToneOutput != nullptr) && animSource != nullptr)
            animSource->addListener (this, AnimationFrameSource::Rate::Hz30);
    }

    ~NoiseComponent() override
    {
        if (animSource != nullptr)
            animSource->removeListener (this);
    }

protected:
    void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const override
    {
        g.setOpacity (isActive ? 1.0f : 0.5f);

        // Draw ghost path at modulated values (behind main)
        if ((modLevelOutput != nullptr || modToneOutput != nullptr)
            && (std::abs (modulatedLevel - param1Value) > 0.001f
                || std::abs (modulatedTone - param2Value) > 0.001f))
        {
            g.setColour (getDrawColor().withAlpha (0.25f));
            drawNoisePath (g, bounds, modulatedLevel, modulatedTone);
        }

        g.setColour (getDrawColor());
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
    std::atomic<float>* modLevelOutput = nullptr;
    std::atomic<float>* modToneOutput = nullptr;
    AnimationFrameSource* animSource = nullptr;
    float modulatedLevel = 0.0f;
    float modulatedTone = 0.0f;

    void onAnimationFrame() override
    {
        constexpr float alpha = 0.3f;
        if (modLevelOutput != nullptr)
        {
            float target = AtomicHelpers::paramLoad (*modLevelOutput);
            modulatedLevel += alpha * (target - modulatedLevel);
        }
        if (modToneOutput != nullptr)
        {
            float target = AtomicHelpers::paramLoad (*modToneOutput);
            modulatedTone += alpha * (target - modulatedTone);
        }
        repaint();
    }

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
