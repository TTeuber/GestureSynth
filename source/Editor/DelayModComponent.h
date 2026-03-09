//
// Created by Tyler Teuber on 3/7/26.
//

#pragma once

#include "Utility/DualParameterComponent.h"
#include "../Utility/Parameters.h"

class DelayModComponent final : public DualParameterComponent
{
public:
    DelayModComponent (const juce::AudioProcessorValueTreeState& apvts,
        const UIContext& ctx = {},
        const juce::String& param1DestID = {},
        const juce::String& param2DestID = {})
        : DualParameterComponent (
              apvts.getParameter (ParamIDs::delayModDepth),
              apvts.getParameter (ParamIDs::delayModRate),
              nullptr,
              ctx,
              param1DestID,
              param2DestID,
              "Modulation")
    {
    }

protected:
    void drawVisualization (juce::Graphics&, const juce::Rectangle<int>&) const override {}

    juce::String getParam1Text() const override
    {
        return formatParameterText (param1, param1Value, "{name}: {value}%");
    }

    juce::String getParam2Text() const override
    {
        return formatParameterText (param2, param2Value, "{name}: {value}%");
    }
};
