//
// Created by Tyler Teuber on 3/19/25.
//

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Utility/AnimationFrameSource.h"
#include "Utility/DualParameterComponent.h"
#include "../Utility/AtomicHelpers.h"
#include "../Utility/Parameters.h"

// First Derived Class (WaveformComponent)
class WaveformComponent final : public DualParameterComponent, public AnimationFrameSource::Listener
{
public:
    WaveformComponent (juce::AudioProcessorValueTreeState& apvts,
        const UIContext& ctx = {},
        std::atomic<float>* modPulseWidthOutputToUse = nullptr,
        std::atomic<float>* modWaveformOutputToUse = nullptr,
        const juce::String& param1DestIDToUse = {},
        const juce::String& param2DestIDToUse = {})
        : DualParameterComponent (
              apvts.getParameter (ParamIDs::oscWaveform),
              apvts.getParameter (ParamIDs::pulseWidth),
              dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (ParamIDs::oscOn)),
              ctx,
              param1DestIDToUse,
              param2DestIDToUse,
              "Oscillator"),
          modPulseWidthOutput (modPulseWidthOutputToUse),
          modWaveformOutput (modWaveformOutputToUse),
          animSource (ctx.animationSource)
    {
        if ((modPulseWidthOutputToUse != nullptr || modWaveformOutputToUse != nullptr) && animSource != nullptr)
            animSource->addListener (this, AnimationFrameSource::Rate::Hz30);
    }

    ~WaveformComponent() override
    {
        if (animSource != nullptr)
            animSource->removeListener (this);
    }

protected:
    void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const override
    {
        g.setOpacity (isActive ? 1.0f : 0.5f);

        const float centerY = static_cast<float> (bounds.getCentreY());

        // Draw ghost path at modulated values (behind main)
        if ((modPulseWidthOutput != nullptr || modWaveformOutput != nullptr)
            && (std::abs (modulatedPulseWidth - param2Value) > 0.001f
                || std::abs (modulatedWaveform - param1Value) > 0.001f))
        {
            g.setColour (getDrawColor().withAlpha (0.25f));
            drawPath (g, bounds, centerY, 0.0f, modulatedWaveform, modulatedPulseWidth);
        }

        // Draw main path on top
        g.setColour (getDrawColor());
        drawPath (g, bounds, centerY, 0.0f, param1Value);
    }

    void drawVisualizationWithValues (juce::Graphics& g,
        const juce::Rectangle<int>& bounds, float p1, float p2) const override
    {
        drawPath (g, bounds, static_cast<float> (bounds.getCentreY()), 0.0f, p1, p2);
    }

private:
    std::atomic<float>* modPulseWidthOutput = nullptr;
    std::atomic<float>* modWaveformOutput = nullptr;
    AnimationFrameSource* animSource = nullptr;
    float modulatedPulseWidth = 0.5f;
    float modulatedWaveform = 0.5f;

    void onAnimationFrame() override
    {
        constexpr float alpha = 0.3f;
        if (modPulseWidthOutput != nullptr)
        {
            float target = AtomicHelpers::paramLoad (*modPulseWidthOutput);
            modulatedPulseWidth += alpha * (target - modulatedPulseWidth);
        }
        if (modWaveformOutput != nullptr)
        {
            float target = AtomicHelpers::paramLoad (*modWaveformOutput);
            modulatedWaveform += alpha * (target - modulatedWaveform);
        }
        repaint();
    }

    void drawPath (juce::Graphics& g, const juce::Rectangle<int>& bounds, const float centerY, const float xOffset, const float waveformMix, const float pulseWidth = -1.0f) const
    {
        const float pw = (pulseWidth >= 0.0f) ? pulseWidth : param2Value;

        // Create waveform path
        juce::Path path;
        const float width = static_cast<float> (bounds.getWidth());
        const float startX = static_cast<float> (bounds.getX()) + xOffset;

        const float amplitude = static_cast<float> (bounds.getHeight()) * padding; // 40% of height for amplitude

        // Define fixed points for consistent slope calculation
        const float topLeft = centerY - amplitude;
        const float bottomRight = centerY + amplitude;

        // Calculate the reference slopes based on a reference pulse width (0.5)
        constexpr float referencePulseWidth = 0.5f;
        const float topSlope = ((centerY - amplitude * waveformMix) - topLeft) / (width * referencePulseWidth);
        const float bottomSlope = (bottomRight - (centerY + amplitude * waveformMix)) / (width * (1.0f - referencePulseWidth));

        // Calculate y positions at the pulse point that maintain these slopes
        const float topPulseY = topLeft + (topSlope * width * pw);
        const float bottomPulseY = bottomRight - (bottomSlope * width * (1.0f - pw));

        // Draw the path
        path.startNewSubPath (startX, topLeft);
        path.lineTo (startX + width * pw, topPulseY);
        path.lineTo (startX + width * pw, bottomPulseY);
        path.lineTo (startX + width, bottomRight);

        // Draw the path
        g.strokePath (path, juce::PathStrokeType (2.0f));
    }
};
