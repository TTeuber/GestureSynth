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
    ModWheelComponent (PluginProcessor& p, ModulationModeState*, AnimationFrameSource* animSource = nullptr)
        : processor (p),
          modWheelRawPtr (p.getSynth().getModWheelRawPtr()),
          animSource (animSource)
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

        g.setColour (SECONDARY_COLOR);
        g.fillRoundedRectangle (trackArea, 4.0f);
        g.setColour (TEXT_COLOR.withAlpha (0.3f));
        g.drawRoundedRectangle (trackArea, 4.0f, 1.0f);

        // Thumb
        float thumbHeight = trackArea.getHeight() * 0.15f;
        float thumbInset = 2.0f;
        float thumbY = trackArea.getY() + (1.0f - currentValue) * (trackArea.getHeight() - thumbHeight);
        auto thumbArea = juce::Rectangle<float> (
            trackArea.getX() + thumbInset,
            thumbY,
            trackArea.getWidth() - thumbInset * 2.0f,
            thumbHeight);

        g.setColour (TEXT_COLOR);
        g.fillRoundedRectangle (thumbArea, 3.0f);
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
class PitchWheelComponent final : public juce::Component, public AnimationFrameSource::Listener,
                                   private juce::AudioProcessorParameter::Listener
{
public:
    explicit PitchWheelComponent (PluginProcessor& p, AnimationFrameSource* animSource = nullptr)
        : processor (p),
          pitchBendRawPtr (p.getSynth().getPitchBendRawPtr()),
          pitchBendRangeParam (p.parameters.getParameter (ParamIDs::pitchBendRange)),
          animSource (animSource)
    {
        if (pitchBendRangeParam != nullptr)
            pitchBendRangeParam->addListener (this);
        if (animSource != nullptr)
            animSource->addListener (this, AnimationFrameSource::Rate::Hz30);
    }

    ~PitchWheelComponent() override
    {
        if (animSource != nullptr)
            animSource->removeListener (this);
        if (pitchBendRangeParam != nullptr)
            pitchBendRangeParam->removeListener (this);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Semitone number at top
        float topHeight = juce::jmin (bounds.getWidth(), 24.0f);
        auto topArea = bounds.removeFromTop (topHeight);
        drawSemitoneNumber (g, topArea);

        bounds.removeFromTop (2.0f);

        // Track
        float trackWidth = bounds.getWidth() * 0.7f;
        auto trackArea = bounds.withSizeKeepingCentre (trackWidth, bounds.getHeight());

        g.setColour (SECONDARY_COLOR);
        g.fillRoundedRectangle (trackArea, 4.0f);
        g.setColour (TEXT_COLOR.withAlpha (0.3f));
        g.drawRoundedRectangle (trackArea, 4.0f, 1.0f);

        // Center line
        float centerY = trackArea.getCentreY();
        g.setColour (TEXT_COLOR.withAlpha (0.3f));
        g.drawLine (trackArea.getX() + 2, centerY, trackArea.getRight() - 2, centerY, 1.0f);

        // Thumb — map 0-16383 to position (0=bottom, 16383=top, 8192=center)
        float normalised = static_cast<float> (currentPitchValue) / 16383.0f;
        float thumbHeight = trackArea.getHeight() * 0.15f;
        float thumbY = trackArea.getY() + (1.0f - normalised) * (trackArea.getHeight() - thumbHeight);
        float thumbInset = 2.0f;
        auto thumbArea = juce::Rectangle<float> (
            trackArea.getX() + thumbInset,
            thumbY,
            trackArea.getWidth() - thumbInset * 2.0f,
            thumbHeight);

        g.setColour (TEXT_COLOR);
        g.fillRoundedRectangle (thumbArea, 3.0f);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        auto bounds = getLocalBounds().toFloat();
        float topHeight = juce::jmin (bounds.getWidth(), 24.0f);

        if (e.y < topHeight)
        {
            isDraggingNumber = true;
            numberDragStartY = e.y;
            numberDragStartValue = getCurrentPitchBendRange();
            if (pitchBendRangeParam != nullptr)
                pitchBendRangeParam->beginChangeGesture();
            return;
        }

        isDraggingTrack = true;
        updatePitchFromMouse (e);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (isDraggingNumber)
        {
            float deltaY = numberDragStartY - static_cast<float> (e.y);
            int steps = static_cast<int> (deltaY / 20.0f);
            int newRange = juce::jlimit (1, 12, numberDragStartValue + steps);
            if (pitchBendRangeParam != nullptr)
            {
                float normalised = pitchBendRangeParam->convertTo0to1 (static_cast<float> (newRange));
                pitchBendRangeParam->setValueNotifyingHost (normalised);
            }
            return;
        }
        if (isDraggingTrack)
            updatePitchFromMouse (e);
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (isDraggingNumber)
        {
            if (pitchBendRangeParam != nullptr)
                pitchBendRangeParam->endChangeGesture();
            isDraggingNumber = false;
            return;
        }
        if (isDraggingTrack)
        {
            isDraggingTrack = false;
            // Spring back to center
            currentPitchValue = 8192;
            AtomicHelpers::paramStore (processor.uiPitchBendValue, 8192);
            repaint();
        }
    }

    void onAnimationFrame() override
    {
        if (isDraggingTrack || pitchBendRawPtr == nullptr)
            return;

        int hw = static_cast<int> (AtomicHelpers::paramLoad (*pitchBendRawPtr));
        if (hw != currentPitchValue)
        {
            currentPitchValue = hw;
            repaint();
        }
    }

    // AudioProcessorParameter::Listener
    void parameterValueChanged (int, float) override
    {
        juce::MessageManager::callAsync ([this] { repaint(); });
    }
    void parameterGestureChanged (int, bool) override {}

private:
    int getCurrentPitchBendRange() const
    {
        if (pitchBendRangeParam != nullptr)
            return static_cast<int> (pitchBendRangeParam->convertFrom0to1 (pitchBendRangeParam->getValue()));
        return 2;
    }

    void drawSemitoneNumber (juce::Graphics& g, juce::Rectangle<float> area)
    {
        int range = getCurrentPitchBendRange();
        g.setColour (TEXT_COLOR);
        g.setFont (14.0f);
        g.drawText (juce::String (range), area, juce::Justification::centred, false);
    }

    void updatePitchFromMouse (const juce::MouseEvent& e)
    {
        auto bounds = getLocalBounds().toFloat();
        float topHeight = juce::jmin (bounds.getWidth(), 24.0f);
        auto trackBounds = bounds.withTrimmedTop (topHeight + 2.0f);

        float thumbHeight = trackBounds.getHeight() * 0.15f;
        float usableHeight = trackBounds.getHeight() - thumbHeight;

        float relY = static_cast<float> (e.y) - trackBounds.getY() - thumbHeight * 0.5f;
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
    juce::RangedAudioParameter* pitchBendRangeParam = nullptr;
    AnimationFrameSource* animSource = nullptr;
    int currentPitchValue = 8192;
    bool isDraggingTrack = false;
    bool isDraggingNumber = false;
    float numberDragStartY = 0.0f;
    int numberDragStartValue = 2;
};
