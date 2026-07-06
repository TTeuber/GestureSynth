//
// Created by Tyler Teuber on 5/10/25.
//

#pragma once

#include "Utility/AnimationFrameSource.h"
#include "Utility/DualParameterComponent.h"
#include "../Utility/AtomicHelpers.h"
#include "../Utility/Parameters.h"

class ChorusComponent final : public DualParameterComponent, public AnimationFrameSource::Listener
{
public:
    ChorusComponent (const juce::AudioProcessorValueTreeState& apvts,
        const UIContext& ctx = {},
        std::atomic<float>* modDepthOutputToUse = nullptr,
        std::atomic<float>* modRateOutputToUse = nullptr,
        const juce::String& param1DestIDToUse = {},
        const juce::String& param2DestIDToUse = {})
        : DualParameterComponent (
              apvts.getParameter (ParamIDs::chorusDepth),
              apvts.getParameter (ParamIDs::chorusRate),
              dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (ParamIDs::chorusOn)),
              ctx,
              param1DestIDToUse,
              param2DestIDToUse,
              "Chorus"),
          modDepthOutput (modDepthOutputToUse),
          modRateOutput (modRateOutputToUse),
          animSource (ctx.animationSource)
    {
        if ((modDepthOutput != nullptr || modRateOutput != nullptr) && animSource != nullptr)
            animSource->addListener (this, AnimationFrameSource::Rate::Hz30);
    }

    ~ChorusComponent() override
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
            drawChorusWaves (g, bounds, modulatedDepth, modulatedRate);
        }

        g.setColour (getDrawColor());
        drawChorusWaves (g, bounds, param1Value, param2Value);
    }

    void drawVisualizationWithValues (juce::Graphics& g,
        const juce::Rectangle<int>& bounds, float p1, float p2) const override
    {
        drawChorusWaves (g, bounds, p1, p2);
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

    static void drawChorusWaves (juce::Graphics& g, const juce::Rectangle<int>& bounds,
        float depth, float rate)
    {
        constexpr float strokeThickness = 1.5f;

        // Calculate wavelength based on rate (higher rate = shorter wavelength)
        const float baseWaveLength = static_cast<float> (bounds.getWidth()) * (1.0f - rate * 0.8f);

        // Calculate amplitude based on depth
        const float maxAmplitude = static_cast<float> (bounds.getHeight()) * 0.40f;
        const float amplitude = maxAmplitude * depth;

        // Draw three sine waves with phase offsets
        constexpr float phase1 = 0.0f;
        constexpr float phase2 = -0.1f;
        constexpr float phase3 = 0.1f;

        drawSineWave (g, bounds, amplitude, baseWaveLength, phase1, strokeThickness);
        drawSineWave (g, bounds, amplitude, baseWaveLength, phase2, strokeThickness / 1.5f);
        drawSineWave (g, bounds, amplitude, baseWaveLength, phase3, strokeThickness / 2.0f);
    }

protected:
    juce::String getParam1Text() const override
    {
        return formatParameterText (param1, param1Value, juce::StringRef ("Chrs Depth: ") + juce::String (param1Value * 30, juce::exactlyEqual (param1Value, 1.0f) ? 0 : 2) + juce::StringRef ("ms"));
    }

    juce::String getParam2Text() const override
    {
        return formatParameterText (param2, param2Value, juce::StringRef ("Chrs Rate: ") + juce::String (param2Value, juce::exactlyEqual (param2Value, 1.0f) ? 0 : 2) + juce::StringRef ("Hz"));
    }

    // The display shows the normalized value directly as "Hz", so parse the
    // typed number straight into normalized [0,1] — bypassing the param's skewed
    // convertTo0to1 that would otherwise re-map it.
    float parseParam2EditedText (const juce::String& text) const override
    {
        auto [numericText, suffix] = InlineParameterEditUtils::splitNumericAndSuffix (
            InlineParameterEditUtils::extractEditableText (text));
        if (numericText.isEmpty())
            return juce::jlimit (0.0f, 1.0f, param2->getValueForText (text));
        return juce::jlimit (0.0f, 1.0f, numericText.getFloatValue());
    }

private:
    static void drawSineWave (const juce::Graphics& g,
        const juce::Rectangle<int>& bounds,
        const float amplitude,
        const float wavelength,
        const float phaseOffset,
        const float strokeThickness)
    {
        // Create a path for the sine wave
        juce::Path path;

        // Calculate the midpoint of the bounds for y-axis
        const float midY = static_cast<float> (bounds.getCentreY());

        // Draw sine wave from left to right
        const float step = juce::jmin (5.0f, wavelength / 20.0f);
        for (float x = 0.0f; x <= static_cast<float> (bounds.getWidth()); x += step)
        {
            // Calculate normalized x-position (0 to 2π)
            const float normalizedX = (x / wavelength) * juce::MathConstants<float>::twoPi;

            // Calculate y-position using sine function with phase offset
            const float y = midY - amplitude * std::sin (normalizedX + phaseOffset * juce::MathConstants<float>::twoPi);

            if (juce::exactlyEqual (x, 0.0f))
            {
                // Start a new path on the left side of the bounds
                path.startNewSubPath (static_cast<float> (bounds.getX()) + x, y);
            }
            else
            {
                // Add a point to a path
                path.lineTo (static_cast<float> (bounds.getX()) + x, y);
            }
        }

        // Stroke the path
        g.strokePath (path, juce::PathStrokeType (strokeThickness));
    }
};