//
// Created by Tyler Teuber on 3/22/25.
//

#pragma once

#include "Modulation.h"

#include <juce_audio_utils/juce_audio_utils.h>
class MyParameter : public juce::AudioProcessorParameter::Listener
{
public:
    explicit MyParameter (juce::RangedAudioParameter* p)
        : parameter (p)
    {
        parameter->addListener (this);
        range = parameter->getNormalisableRange();
        baseValue = parameter->getValue();
        minValue = range.start;
        maxValue = range.end;
        id = parameter->getParameterID();
        name = parameter->getName (20);
    }
    // MyParameter (juce::RangedAudioParameter* p, const float min, const float max, const float value)
    //     : parameter (p),
    //       baseValue (value),
    //       minValue (min),
    //       maxValue (max)
    // {
    //     parameter->addListener (this);
    //     range = parameter->getNormalisableRange();
    //     range.start = min;
    //     range.end = max;
    //     parameter->setValueNotifyingHost (value);
    //     id = parameter->getParameterID();
    //     name = parameter->getName (20);
    // }

    ~MyParameter() override
    {
        parameter->removeListener (this);
    }

    void parameterValueChanged (int /*parameterIndex*/, const float newValue) override
    {
        baseValue = newValue;
    }

    void parameterGestureChanged (int /*parameterIndex*/, bool gestureIsStarting) override
    {
        // if (gestureIsStarting)
        //     value = currentValue;
    }

    [[nodiscard]] virtual float getMinValue() const noexcept { return minValue; }
    [[nodiscard]] virtual float getMaxValue() const noexcept { return maxValue; }
    [[nodiscard]] virtual juce::NormalisableRange<float> getRange() const noexcept { return range; }

protected:
    juce::RangedAudioParameter* parameter;

    juce::String id;
    juce::String name;
    float baseValue;
    float minValue;
    float maxValue;
    juce::NormalisableRange<float> range;
};

class StaticParameter final : public MyParameter
{
public:
    explicit StaticParameter (juce::RangedAudioParameter* p)
        : MyParameter (p)
    {
    }
    // StaticParameter (juce::RangedAudioParameter* p, const float min, const float max, const float value)
    //     : MyParameter (p, min, max, value)
    // {
    // }
    void setValue (const float newValue)
    {
        baseValue = newValue;
    }
    [[nodiscard]] float getValue() const
    {
        return baseValue;
    }
};

class DynamicParameter final : MyParameter, public ModDestination
{
public:
    explicit DynamicParameter (juce::RangedAudioParameter* p)
        : MyParameter (p), ModDestination (p->getParameterID(), p->getName (20))
    {
        currentValue = baseValue;
    }
    // DynamicParameter (juce::RangedAudioParameter* p, const float min, const float max, const float value)
    //     : MyParameter (p, min, max, value), ModDestination (p->getParameterID(), p->getName (20))
    // {
    //     currentValue = baseValue;
    // }

    [[nodiscard]] float getBaseValue() const noexcept override { return baseValue; }
    void setBaseValue (const float value) noexcept override { baseValue = value; }
    [[nodiscard]] float getCurrentValue() const noexcept override { return currentValue; }
    void setCurrentValue (const float value) noexcept override { currentValue = value; }
    [[nodiscard]] float getMinValue() const noexcept override { return minValue; }
    [[nodiscard]] float getMaxValue() const noexcept override { return maxValue; }
    [[nodiscard]] juce::NormalisableRange<float> getRange() const noexcept override { return range; }

private:
    float currentValue;
};
