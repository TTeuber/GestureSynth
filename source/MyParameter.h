//
// Created by Tyler Teuber on 3/22/25.
//

#pragma once

#include "Modulation.h"

#include <juce_audio_utils/juce_audio_utils.h>

class MyParameter : public ModDestination, public juce::AudioProcessorParameter::Listener
{
public:
    MyParameter (juce::RangedAudioParameter* p, const float min, const float max, const float value)
        : parameter (p),
          baseValue (value),
          currentValue (value),
          minValue (min),
          maxValue (max)
    {
        parameter->addListener (this);
        range = parameter->getNormalisableRange();
    }

    void parameterValueChanged (int parameterIndex, const float newValue) override
    {
        baseValue = newValue;
    }

    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override
    {
        if (gestureIsStarting)
            baseValue = currentValue;
    }

    [[nodiscard]] float getBaseValue() const noexcept override { return baseValue; }
    void setBaseValue (const float value) noexcept override { baseValue = value; }
    [[nodiscard]] float getCurrentValue() const noexcept override { return currentValue; };
    void setCurrentValue (const float value) noexcept override { currentValue = value; }
    [[nodiscard]] float getMinValue() const noexcept override { return minValue; }
    [[nodiscard]] float getMaxValue() const noexcept override { return maxValue; }
    [[nodiscard]] juce::NormalisableRange<float> getRange() const noexcept override { return range; }

private:
    juce::RangedAudioParameter* parameter;

    float baseValue;
    float currentValue;
    float minValue;
    float maxValue;
    juce::NormalisableRange<float> range;
};
