#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Theme.h"
#include "PaintHelpers.h"

class CustomToggleComponent final : public juce::Component,
                                     private juce::AudioProcessorParameter::Listener
{
public:
    explicit CustomToggleComponent (juce::AudioParameterBool* paramToUse, const juce::String& labelToUse)
        : param (paramToUse), label (labelToUse)
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

    void setAccentColor (juce::Colour color)
    {
        accentColor = color;
        useAccent = true;
        repaint();
    }

    void setCornerRadius (float radius)
    {
        cornerRadius = radius;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const float opacity = (opacityCallback && !opacityCallback()) ? 0.35f : 1.0f;
        g.setOpacity (opacity);

        auto bounds = getLocalBounds().toFloat();

        if (useAccent)
        {
            if (active)
            {
                g.setColour (TERTIARY_COLOR.interpolatedWith (accentColor, 0.15f));
                g.fillRoundedRectangle (bounds, cornerRadius);
                g.setColour (accentColor.withAlpha (0.4f));
                g.drawRoundedRectangle (bounds.reduced (0.5f), cornerRadius, 1.5f);
            }
            else
            {
                g.setColour (TERTIARY_COLOR);
                g.fillRoundedRectangle (bounds, cornerRadius);
            }
            g.setColour (TEXT_COLOR);
        }
        else if (active)
        {
            g.setColour (TEXT_COLOR);
            g.fillRoundedRectangle (bounds, cornerRadius);
            g.setColour (PRIMARY_COLOR);
        }
        else
        {
            PaintHelpers::drawComponentBox (g, bounds, cornerRadius, 1.0f);
            g.setColour (TEXT_COLOR);
        }

        g.setFont (Style::fontBody);
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
    juce::Colour accentColor;
    bool useAccent = false;
    float cornerRadius = Style::radiusLarge;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomToggleComponent)
};
