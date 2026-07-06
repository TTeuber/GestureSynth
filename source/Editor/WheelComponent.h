#pragma once

#include "../PluginProcessor.h"
#include "../Theme.h"
#include "../Utility/AtomicHelpers.h"
#include "../Utility/Parameters.h"
#include "Utility/AnimationFrameSource.h"
#include "Utility/ModulationModeState.h"

// =============================================================================
// ModWheelComponent
// =============================================================================
class ModWheelComponent final : public juce::Component, public AnimationFrameSource::Listener
{
public:
    ModWheelComponent (PluginProcessor& p, ModulationModeState*, AnimationFrameSource* animSourceToUse = nullptr)
        : processor (p),
          modWheelRawPtr (p.getSynth().getModWheelRawPtr()),
          animSource (animSourceToUse)
    {
        if (animSource != nullptr)
            animSource->addListener (this, AnimationFrameSource::Rate::Hz30);
    }

    ~ModWheelComponent() override
    {
        if (animSource != nullptr)
            animSource->removeListener (this);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Track
        float trackWidth = bounds.getWidth() * 0.7f;
        auto trackArea = bounds.withSizeKeepingCentre (trackWidth, bounds.getHeight());

        g.setColour (TERTIARY_COLOR);
        g.fillRoundedRectangle (trackArea, Style::radiusMedium);

        // Thumb
        float thumbHeight = trackArea.getHeight() * 0.15f;
        float thumbInset = 1.0f;
        float thumbY = trackArea.getY() + (1.0f - currentValue) * (trackArea.getHeight() - thumbHeight);
        auto thumbArea = juce::Rectangle<float> (
            trackArea.getX() + thumbInset,
            thumbY,
            trackArea.getWidth() - thumbInset * 2.0f,
            thumbHeight);

        g.setColour (TEXT_COLOR);
        g.fillRoundedRectangle (thumbArea, Style::radiusSmall);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        isDragging = true;
        updateValueFromMouse (e);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (!isDragging)
            return;
        updateValueFromMouse (e);
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        isDragging = false;
        // Mod wheel stays where released (no spring-back)
    }

    void onAnimationFrame() override
    {
        if (isDragging || modWheelRawPtr == nullptr)
            return;

        float hw = AtomicHelpers::paramLoad (*modWheelRawPtr);
        if (std::abs (hw - currentValue) > 0.001f)
        {
            currentValue = hw;
            repaint();
        }
    }

private:
    void updateValueFromMouse (const juce::MouseEvent& e)
    {
        auto bounds = getLocalBounds().toFloat();

        float thumbHeight = bounds.getHeight() * 0.15f;
        float usableHeight = bounds.getHeight() - thumbHeight;

        float relY = static_cast<float> (e.y) - bounds.getY() - thumbHeight * 0.5f;
        float rawVal = 1.0f - juce::jlimit (0.0f, 1.0f, relY / usableHeight);

        if (e.mods.isShiftDown())
            rawVal = currentValue + (rawVal - currentValue) * 0.1f;

        currentValue = juce::jlimit (0.0f, 1.0f, rawVal);
        AtomicHelpers::paramStore (processor.uiModWheelValue, currentValue);
        if (modWheelRawPtr != nullptr)
            AtomicHelpers::paramStore (*modWheelRawPtr, currentValue);
        repaint();
    }

    PluginProcessor& processor;
    std::atomic<float>* modWheelRawPtr = nullptr;
    AnimationFrameSource* animSource = nullptr;
    float currentValue = 0.0f;
    bool isDragging = false;
};

// =============================================================================
// PitchWheelComponent
// =============================================================================
class PitchWheelComponent final : public juce::Component, public AnimationFrameSource::Listener
{
public:
    explicit PitchWheelComponent (PluginProcessor& p, AnimationFrameSource* animSourceToUse = nullptr)
        : processor (p),
          pitchBendRawPtr (p.getSynth().getPitchBendRawPtr()),
          animSource (animSourceToUse)
    {
        if (animSource != nullptr)
            animSource->addListener (this, AnimationFrameSource::Rate::Hz30);
    }

    ~PitchWheelComponent() override
    {
        if (animSource != nullptr)
            animSource->removeListener (this);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Track
        float trackWidth = bounds.getWidth() * 0.7f;
        auto trackArea = bounds.withSizeKeepingCentre (trackWidth, bounds.getHeight());

        g.setColour (TERTIARY_COLOR);
        g.fillRoundedRectangle (trackArea, Style::radiusMedium);

        // Center line
        float centerY = trackArea.getCentreY();
        g.setColour (TEXT_COLOR.withAlpha (Style::alphaBorder / 2.0f));
        g.drawLine (trackArea.getX() + 2, centerY, trackArea.getRight() - 2, centerY, 1.0f);

        // Thumb — map 0-16383 to position (0=bottom, 16383=top, 8192=center)
        float normalised = static_cast<float> (currentPitchValue) / 16383.0f;
        float thumbHeight = trackArea.getHeight() * 0.15f;
        float thumbY = trackArea.getY() + (1.0f - normalised) * (trackArea.getHeight() - thumbHeight);
        float thumbInset = 1.0f;
        auto thumbArea = juce::Rectangle<float> (
            trackArea.getX() + thumbInset,
            thumbY,
            trackArea.getWidth() - thumbInset * 2.0f,
            thumbHeight);

        g.setColour (TEXT_COLOR);
        g.fillRoundedRectangle (thumbArea, Style::radiusSmall);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        isDragging = true;
        updatePitchFromMouse (e);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (isDragging)
            updatePitchFromMouse (e);
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (isDragging)
        {
            isDragging = false;
            // Spring back to center
            currentPitchValue = 8192;
            AtomicHelpers::paramStore (processor.uiPitchBendValue, 8192);
            if (pitchBendRawPtr != nullptr)
                AtomicHelpers::paramStore (*pitchBendRawPtr, 8192.0f);
            repaint();
        }
    }

    void onAnimationFrame() override
    {
        if (isDragging || pitchBendRawPtr == nullptr)
            return;

        int hw = static_cast<int> (AtomicHelpers::paramLoad (*pitchBendRawPtr));
        if (hw != currentPitchValue)
        {
            currentPitchValue = hw;
            repaint();
        }
    }

private:
    void updatePitchFromMouse (const juce::MouseEvent& e)
    {
        auto bounds = getLocalBounds().toFloat();

        float thumbHeight = bounds.getHeight() * 0.15f;
        float usableHeight = bounds.getHeight() - thumbHeight;

        float relY = static_cast<float> (e.y) - bounds.getY() - thumbHeight * 0.5f;
        float normalised = 1.0f - juce::jlimit (0.0f, 1.0f, relY / usableHeight);

        if (e.mods.isShiftDown())
        {
            float centerNorm = 0.5f;
            normalised = centerNorm + (normalised - centerNorm) * 0.1f;
        }

        currentPitchValue = static_cast<int> (normalised * 16383.0f);
        AtomicHelpers::paramStore (processor.uiPitchBendValue, currentPitchValue);
        repaint();
    }

    PluginProcessor& processor;
    std::atomic<float>* pitchBendRawPtr = nullptr;
    AnimationFrameSource* animSource = nullptr;
    int currentPitchValue = 8192;
    bool isDragging = false;
};
