//
// Created by Tyler Teuber on 3/22/25.
//
#pragma once

#include "LFOData.h"
#include "Modulation.h"
#include <algorithm>
#include <functional>
#include <juce_dsp/juce_dsp.h>
#include <vector>

/**
 * LFO class implementing the ModSource interface
 */
class MyLFO final : public ModSource
{
public:
    explicit MyLFO (const juce::String& id = "lfo", const juce::String& name = "LFO", const float rate = 1.0f, std::shared_ptr<LFOData> data = nullptr)
        : ModSource (id, name),
          rate (rate),
          phase (0.0f),
          currentValue (0.0f),
          lfoData (data ? data : std::make_shared<LFOData>())
    {
    }

    /**
     * Sets the LFO rate in Hz
     */
    void setRate (const float newRate) noexcept
    {
        rate = juce::jlimit (0.01f, 100.0f, newRate);
        updatePhaseIncrement();
    }

    /**
     * Gets the current LFO rate in Hz
     */
    [[nodiscard]] float getRate() const noexcept { return rate; }

    /**
     * Sets the LFO shape to one of the predefined shapes
     */
    void setShape (const LFOShape shape)
    {
        lfoData->setShape (shape);
    }

    /**
     * Gets the current value of the LFO
     */
    [[nodiscard]] float getCurrentValue() const noexcept override
    {
        return currentValue;
    }

    /**
     * Generates the next value in the LFO sequence
     */
    float getNextValue() noexcept override
    {
        // Calculate the value at the current phase position
        currentValue = lfoData->getValueAt (phase);

        // Update the phase for the next sample
        phase += phaseIncrement;
        if (phase >= 1.0f)
            phase -= 1.0f;

        return currentValue;
    }

    /**
     * Prepares the LFO for processing
     */
    void prepare (const juce::dsp::ProcessSpec& spec) override
    {
        ModSource::prepare (spec);
        updatePhaseIncrement();
    }

    /**
     * Resets the LFO phase to 0
     */
    void reset() noexcept
    {
        phase = 0.0f;
    }

    /**
     * Sets the LFO phase (0.0 to 1.0)
     */
    void setPhase (const float newPhase) noexcept
    {
        phase = juce::jlimit (0.0f, 1.0f, newPhase);
    }

    [[nodiscard]] float getPhase() const noexcept { return phase; }

    /**
     * Gets the shared LFO data pointer
     */
    std::shared_ptr<LFOData> getLFOData() const noexcept
    {
        return lfoData;
    }

    /**
     * Sets the shared LFO data pointer
     */
    void setLFOData (std::shared_ptr<LFOData> newData)
    {
        if (newData)
            lfoData = newData;
    }

private:
    /**
     * Updates the phase increment based on the current rate and sample rate
     */
    void updatePhaseIncrement() noexcept
    {
        phaseIncrement = rate / sampleRate;
    }

    float rate; // LFO rate in Hz
    float phase; // Current phase (0.0 to 1.0)
    float phaseIncrement; // Phase increment per sample
    float currentValue; // Current output value
    std::shared_ptr<LFOData> lfoData; // Shared LFO data that can be edited by UI
};