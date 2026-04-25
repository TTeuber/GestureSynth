#pragma once

#include "Utility/TextParameterComponent.h"
#include "../Utility/TempoSyncUtils.h"

class LFORateComponent final : public TextParameterComponent
{
public:
    LFORateComponent (juce::RangedAudioParameter* rateParam,
                      juce::AudioParameterChoice* noteDivParam,
                      juce::AudioParameterBool* tempoSyncParam)
        : TextParameterComponent (rateParam),
          noteDivParam (noteDivParam),
          tempoSyncParam (tempoSyncParam),
          extraListener (*this)
    {
        if (noteDivParam != nullptr)
            noteDivParam->addListener (&extraListener);
        if (tempoSyncParam != nullptr)
            tempoSyncParam->addListener (&extraListener);
    }

    ~LFORateComponent() override
    {
        if (noteDivParam != nullptr)
            noteDivParam->removeListener (&extraListener);
        if (tempoSyncParam != nullptr)
            tempoSyncParam->removeListener (&extraListener);
    }

    void rebind (juce::RangedAudioParameter* newRateParam,
                 juce::AudioParameterChoice* newNoteDivParam,
                 juce::AudioParameterBool* newTempoSyncParam)
    {
        // Remove old listeners from extra params
        if (noteDivParam != nullptr)
            noteDivParam->removeListener (&extraListener);
        if (tempoSyncParam != nullptr)
            tempoSyncParam->removeListener (&extraListener);

        rebindParam (newRateParam);

        noteDivParam = newNoteDivParam;
        tempoSyncParam = newTempoSyncParam;

        if (noteDivParam != nullptr)
            noteDivParam->addListener (&extraListener);
        if (tempoSyncParam != nullptr)
            tempoSyncParam->addListener (&extraListener);

        repaint();
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (isBpmMode())
        {
            divDragStartY = e.y;
            divDragStartIndex = noteDivParam->getIndex();
            isDivDragging = true;
            return;
        }
        TextParameterComponent::mouseDown (e);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (isDivDragging && noteDivParam != nullptr)
        {
            int delta = (divDragStartY - e.y) / 20;
            int newIndex = juce::jlimit (0, TempoSync::numDivisions - 1, divDragStartIndex + delta);
            if (newIndex != noteDivParam->getIndex())
            {
                noteDivParam->beginChangeGesture();
                noteDivParam->setValueNotifyingHost (
                    noteDivParam->convertTo0to1 (static_cast<float> (newIndex)));
                noteDivParam->endChangeGesture();
            }
            return;
        }
        TextParameterComponent::mouseDrag (e);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (isDivDragging)
        {
            isDivDragging = false;
            return;
        }
        TextParameterComponent::mouseUp (e);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        g.setColour (TERTIARY_COLOR);
        g.fillRoundedRectangle (bounds, Style::radiusLarge);

        g.setColour (TEXT_COLOR.withAlpha (0.6f));
        g.setFont (Style::fontCaption);
        g.drawText ("Rate", bounds.reduced (4.0f, 2.0f),
            juce::Justification::centredTop, true);

        auto valueArea = bounds.reduced (4.0f).withTrimmedTop (14.0f);
        drawValueText (g, valueArea);
    }

protected:
    void drawValueText (juce::Graphics& g, juce::Rectangle<float> area) const override
    {
        g.setColour (TEXT_COLOR);

        if (isBpmMode())
        {
            drawFractionText (g, area);
        }
        else
        {
            float hz = param->convertFrom0to1 (paramValue);
            juce::String text;
            if (hz < 10.0f)
                text = juce::String (hz, 1) + " Hz";
            else
                text = juce::String (static_cast<int> (hz)) + " Hz";
            g.setFont (Style::fontLabel);
            g.drawText (text, area, juce::Justification::centred, true);
        }
    }

private:
    bool isBpmMode() const
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

        juce::String suffix;
        if (isDotted) suffix = ".";
        else if (isTriplet) suffix = "T";

        juce::String displayText = cleanDiv + suffix;

        g.setFont (Style::fontBody);
        g.drawText (displayText, area, juce::Justification::centred, true);
    }

    // Helper listener to avoid diamond inheritance with AudioProcessorParameter::Listener
    struct ExtraParamListener : juce::AudioProcessorParameter::Listener
    {
        explicit ExtraParamListener (LFORateComponent& owner) : owner (owner) {}
        void parameterValueChanged (int, float) override
        {
            juce::MessageManager::callAsync ([&owner = this->owner] { owner.repaint(); });
        }
        void parameterGestureChanged (int, bool) override {}
        LFORateComponent& owner;
    };

    ExtraParamListener extraListener;
    juce::AudioParameterChoice* noteDivParam = nullptr;
    juce::AudioParameterBool* tempoSyncParam = nullptr;

    bool isDivDragging = false;
    int divDragStartY = 0;
    int divDragStartIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LFORateComponent)
};
