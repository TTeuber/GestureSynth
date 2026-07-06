//
// Created by Tyler Teuber on 2/10/26.
//

#pragma once

#include "Utility/AnimationFrameSource.h"
#include "Utility/DualParameterComponent.h"
#include "../Utility/AtomicHelpers.h"
#include "../Utility/Parameters.h"

class VibratoComponent final : public DualParameterComponent, public AnimationFrameSource::Listener
{
public:
    VibratoComponent (const juce::AudioProcessorValueTreeState& apvts,
        const UIContext& ctx = {},
        std::atomic<float>* modDepthOutputToUse = nullptr,
        std::atomic<float>* modRateOutputToUse = nullptr,
        const juce::String& param1DestIDToUse = {},
        const juce::String& param2DestIDToUse = {})
        : DualParameterComponent (
              apvts.getParameter (ParamIDs::vibratoDepth),
              apvts.getParameter (ParamIDs::vibratoRate),
              dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (ParamIDs::vibratoOn)),
              ctx,
              param1DestIDToUse,
              param2DestIDToUse,
              "Vibrato"),
          modDepthOutput (modDepthOutputToUse),
          modRateOutput (modRateOutputToUse),
          animSource (ctx.animationSource)
    {
        if ((modDepthOutput != nullptr || modRateOutput != nullptr) && animSource != nullptr)
            animSource->addListener (this, AnimationFrameSource::Rate::Hz30);
    }

    ~VibratoComponent() override
    {
        if (animSource != nullptr)
            animSource->removeListener (this);
    }

protected:
    void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const override
    {
        g.setOpacity (isActive ? 1.0f : 0.5f);

        // Draw ghost path at modulated values (behind main)
        if ((modDepthOutput != nullptr || modRateOutput != nullptr)
            && (std::abs (modulatedDepth - param1Value) > 0.001f
                || std::abs (modulatedRate - param2Value) > 0.001f))
        {
            g.setColour (getDrawColor().withAlpha (0.25f));
            drawVibratoPath (g, bounds, modulatedDepth, modulatedRate);
        }

        g.setColour (getDrawColor());
        drawVibratoPath (g, bounds, param1Value, param2Value);
    }

    void drawVisualizationWithValues (juce::Graphics& g,
        const juce::Rectangle<int>& bounds, float p1, float p2) const override
    {
        drawVibratoPath (g, bounds, p1, p2);
    }

private:
    std::atomic<float>* modDepthOutput = nullptr;
    std::atomic<float>* modRateOutput = nullptr;
    AnimationFrameSource* animSource = nullptr;
    float modulatedDepth = 0.0f;
    float modulatedRate = 0.0f;

    void onAnimationFrame() override
    {
        constexpr float alpha = 0.3f;
        if (modDepthOutput != nullptr)
        {
            float target = AtomicHelpers::paramLoad (*modDepthOutput);
            modulatedDepth += alpha * (target - modulatedDepth);
        }
        if (modRateOutput != nullptr)
        {
            float target = AtomicHelpers::paramLoad (*modRateOutput);
            modulatedRate += alpha * (target - modulatedRate);
        }
        repaint();
    }

    static void drawVibratoPath (juce::Graphics& g, const juce::Rectangle<int>& bounds,
        float depth, float rate)
    {
        constexpr float strokeThickness = 1.5f;

        const float rateFactor = 0.25f + rate * 0.5f;
        const float normFactor = 0.75f / (1.0f + depth);

        juce::Path path;
        const float midY = static_cast<float> (bounds.getCentreY());
        const float amplitude = static_cast<float> (bounds.getHeight()) * 0.40f;
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

protected:
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
