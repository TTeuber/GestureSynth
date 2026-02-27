#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Theme.h"

class CustomToggleComponent final : public juce::Component,
                                     private juce::AudioProcessorParameter::Listener
{
public:
    explicit CustomToggleComponent (juce::AudioParameterBool* param, const juce::String& label)
        : param (param), label (label)
    {
        jassert (param != nullptr);
        param->addListener (this);
        active = param->get();
    }

    ~CustomToggleComponent() override
    {
        if (param != nullptr)
            param->removeListener (this);
    }

    void rebind (juce::AudioParameterBool* newParam)
    {
        if (param != nullptr)
            param->removeListener (this);
        param = newParam;
        if (param != nullptr)
        {
            param->addListener (this);
            active = param->get();
        }
        repaint();
    }

    void setOpacityCallback (std::function<bool()> cb)
    {
        opacityCallback = std::move (cb);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const float opacity = (opacityCallback && !opacityCallback()) ? 0.35f : 1.0f;
        g.setOpacity (opacity);

        auto bounds = getLocalBounds().toFloat();

        if (active)
        {
            g.setColour (TEXT_COLOR);
            g.fillRoundedRectangle (bounds, 6.0f);
            g.setColour (PRIMARY_COLOR);
        }
        else
        {
            g.setColour (SECONDARY_COLOR);
            g.fillRoundedRectangle (bounds, 6.0f);
            g.setColour (TEXT_COLOR);
            g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);
        }

        g.setFont (12.0f);
        g.drawText (label, bounds, juce::Justification::centred, true);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (param != nullptr && getLocalBounds().contains (e.x, e.y))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost (active ? 0.0f : 1.0f);
            param->endChangeGesture();
        }
    }

private:
    void parameterValueChanged (int, float) override
    {
        active = param->get();
        juce::MessageManager::callAsync ([this] { repaint(); });
    }

    void parameterGestureChanged (int, bool) override {}

    juce::AudioParameterBool* param = nullptr;
    juce::String label;
    bool active = false;
    std::function<bool()> opacityCallback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomToggleComponent)
};
