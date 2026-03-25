#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Theme.h"

class SynthEffectsToggle final : public juce::Component
{
public:
    SynthEffectsToggle() = default;

    void setOnChange (std::function<void (bool)> cb) { onChange = std::move (cb); }

    bool isShowingEffects() const { return showingEffects; }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        if (showingEffects)
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
        g.drawText (showingEffects ? "Synth" : "Effects", bounds, juce::Justification::centred, true);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (getLocalBounds().contains (e.x, e.y))
        {
            showingEffects = !showingEffects;
            repaint();
            if (onChange)
                onChange (showingEffects);
        }
    }

private:
    bool showingEffects = false;
    std::function<void (bool)> onChange;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthEffectsToggle)
};
