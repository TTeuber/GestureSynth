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
                        juce::AudioParameterBool* activeParam = nullptr,
                        const UIContext& ctx = {})
        : SingleParameterComponent (timeParam, activeParam, ctx),
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
        if (isSyncMode() && !e.mods.isPopupMenu() && isActive)
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
            int newIndex = juce::jlimit (0, numDivisions - 1, divDragStartIndex - delta);
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
        float arcParam = isSyncMode() && noteDivParam != nullptr
            ? 1.0f - noteDivParam->convertTo0to1 (static_cast<float> (noteDivParam->getIndex()))
            : paramValue;

        g.setOpacity (isActive ? 1.0f : 0.5f);
        g.setColour (isActive ? TEXT_COLOR : TEXT_INACTIVE_COLOR);
        drawTimeArcs (g, bounds, arcParam);
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
    static void drawTimeArcs (juce::Graphics& g, const juce::Rectangle<int>& bounds,
        float paramVal)
    {
        const float numArcsF = 3.0f + 5.0f * (1.0f - paramVal);
        const int numArcs = static_cast<int> (std::ceil (numArcsF));
        const float width = static_cast<float> (bounds.getWidth());
        const float height = static_cast<float> (bounds.getHeight());
        const float left = static_cast<float> (bounds.getX()) - width * 0.08f;
        const float usableWidth = width * 1.08f;
        const float centerY = static_cast<float> (bounds.getY()) + height * 0.5f;
        const float baseHeight = height * 0.8f;
        const float decayRate = 0.4f;
        const float spacing = usableWidth / (numArcsF + 1.0f);
        const float baseThickness = 3.0f;

        for (int i = 0; i < numArcs; ++i)
        {
            const float fi = static_cast<float> (i);
            const float fractional = juce::jlimit (0.0f, 1.0f, numArcsF - fi);
            const float decayFactor = std::exp (-decayRate * fi);
            const float arcX = left + spacing * (fi + 1.0f);
            const float arcHeight = baseHeight * decayFactor * fractional;
            const float halfHeight = arcHeight * 0.5f;
            const float curveDepth = spacing * 0.4f * std::max (0.3f, decayFactor);
            const float thickness = std::max (1.0f, baseThickness * std::exp (-0.3f * fi));

            juce::Path arc;
            arc.startNewSubPath (arcX, centerY - halfHeight);
            arc.quadraticTo (arcX + curveDepth, centerY, arcX, centerY + halfHeight);

            g.strokePath (arc, juce::PathStrokeType (thickness, juce::PathStrokeType::curved,
                juce::PathStrokeType::rounded));
        }
    }

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
        g.setFont (Style::fontBody);
        g.drawText (numerator, area.removeFromTop (lineHeight), juce::Justification::centred, true);
        g.setFont (Style::fontCaption);
        auto slashArea = area.removeFromTop (lineHeight);
        g.drawText ("/", slashArea, juce::Justification::centred, true);
        g.setFont (Style::fontBody);
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
