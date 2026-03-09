//
// Created by Tyler Teuber on 3/7/26.
//

#pragma once

#include "Utility/SingleParameterComponent.h"
#include "../Utility/Parameters.h"
#include "../Utility/TempoSyncUtils.h"

class DelayRateComponent final : public SingleParameterComponent
{
public:
    DelayRateComponent (juce::RangedAudioParameter* timeParam,
                        juce::AudioParameterBool* tempoSyncParam,
                        juce::AudioParameterChoice* noteDivParam,
                        const UIContext& ctx = {})
        : SingleParameterComponent (timeParam, nullptr, ctx),
          tempoSyncParam (tempoSyncParam),
          noteDivParam (noteDivParam),
          extraListener (*this)
    {
        if (tempoSyncParam != nullptr)
            tempoSyncParam->addListener (&extraListener);
        if (noteDivParam != nullptr)
            noteDivParam->addListener (&extraListener);
    }

    ~DelayRateComponent() override
    {
        if (tempoSyncParam != nullptr)
            tempoSyncParam->removeListener (&extraListener);
        if (noteDivParam != nullptr)
            noteDivParam->removeListener (&extraListener);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (isSyncMode())
        {
            divDragStartY = e.y;
            divDragStartIndex = noteDivParam->getIndex();
            isDivDragging = true;
            return;
        }
        SingleParameterComponent::mouseDown (e);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (isDivDragging && noteDivParam != nullptr)
        {
            int numDivisions = noteDivParam->choices.size();
            int delta = (divDragStartY - e.y) / 20;
            int newIndex = juce::jlimit (0, numDivisions - 1, divDragStartIndex + delta);
            if (newIndex != noteDivParam->getIndex())
            {
                noteDivParam->beginChangeGesture();
                noteDivParam->setValueNotifyingHost (
                    noteDivParam->convertTo0to1 (static_cast<float> (newIndex)));
                noteDivParam->endChangeGesture();
            }
            return;
        }
        SingleParameterComponent::mouseDrag (e);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (isDivDragging)
        {
            isDivDragging = false;
            return;
        }
        SingleParameterComponent::mouseUp (e);
    }

protected:
    void drawVisualization (juce::Graphics& g, const juce::Rectangle<int>& bounds) const override
    {
        if (isSyncMode() && noteDivParam != nullptr)
        {
            g.setColour (isActive ? TEXT_COLOR : TEXT_INACTIVE_COLOR);
            drawFractionText (g, bounds.toFloat());
        }
        else
        {
            // Show delay time in ms
            float ms = param->convertFrom0to1 (paramValue);
            juce::String text;
            if (ms < 10.0f)
                text = juce::String (ms, 1) + " ms";
            else
                text = juce::String (static_cast<int> (ms)) + " ms";

            g.setColour (isActive ? TEXT_COLOR : TEXT_INACTIVE_COLOR);
            g.setFont (13.0f);
            g.drawText (text, bounds, juce::Justification::centred, true);
        }
    }

    juce::String getParameterText() const override
    {
        if (isSyncMode() && noteDivParam != nullptr)
            return noteDivParam->getCurrentValueAsText();

        float ms = param->convertFrom0to1 (paramValue);
        if (ms < 10.0f)
            return juce::String (ms, 1) + " ms";
        return juce::String (static_cast<int> (ms)) + " ms";
    }

private:
    bool isSyncMode() const
    {
        return tempoSyncParam != nullptr && tempoSyncParam->get();
    }

    void drawFractionText (juce::Graphics& g, juce::Rectangle<float> area) const
    {
        if (noteDivParam == nullptr)
            return;

        auto divName = noteDivParam->getCurrentValueAsText();

        bool isDotted = divName.endsWithIgnoreCase ("D");
        bool isTriplet = divName.endsWithIgnoreCase ("T");

        juce::String cleanDiv = divName;
        if (isDotted || isTriplet)
            cleanDiv = divName.dropLastCharacters (1);

        juce::String numerator, denominator;
        if (cleanDiv.contains ("/"))
        {
            numerator = cleanDiv.upToFirstOccurrenceOf ("/", false, false);
            denominator = cleanDiv.fromFirstOccurrenceOf ("/", false, false);
        }
        else
        {
            numerator = cleanDiv;
            denominator = "";
        }

        juce::String suffix;
        if (isDotted) suffix = ".";
        else if (isTriplet) suffix = "T";

        float lineHeight = area.getHeight() / 3.0f;
        g.setFont (12.0f);
        g.drawText (numerator, area.removeFromTop (lineHeight), juce::Justification::centred, true);
        g.setFont (10.0f);
        auto slashArea = area.removeFromTop (lineHeight);
        g.drawText ("/", slashArea, juce::Justification::centred, true);
        g.setFont (12.0f);
        g.drawText (denominator + suffix, area, juce::Justification::centred, true);
    }

    struct ExtraParamListener : juce::AudioProcessorParameter::Listener
    {
        explicit ExtraParamListener (DelayRateComponent& owner) : owner (owner) {}
        void parameterValueChanged (int, float) override
        {
            juce::MessageManager::callAsync ([&owner = this->owner] { owner.repaint(); });
        }
        void parameterGestureChanged (int, bool) override {}
        DelayRateComponent& owner;
    };

    ExtraParamListener extraListener;
    juce::AudioParameterBool* tempoSyncParam = nullptr;
    juce::AudioParameterChoice* noteDivParam = nullptr;

    bool isDivDragging = false;
    int divDragStartY = 0;
    int divDragStartIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayRateComponent)
};
