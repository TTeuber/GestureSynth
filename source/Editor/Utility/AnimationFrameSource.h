#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class AnimationFrameSource final : private juce::Timer
{
public:
    struct Listener
    {
        virtual ~Listener() = default;
        virtual void onAnimationFrame() = 0;
    };

    enum class Rate { Hz30, Hz60 };

    void addListener (Listener* l, Rate rate)
    {
        if (rate == Rate::Hz60)
            listeners60.addIfNotAlreadyThere (l);
        else
            listeners30.addIfNotAlreadyThere (l);
    }

    void removeListener (Listener* l)
    {
        listeners60.removeFirstMatchingValue (l);
        listeners30.removeFirstMatchingValue (l);
    }

    void start() { startTimerHz (60); }
    void stop()  { stopTimer(); }

private:
    void timerCallback() override
    {
        for (auto* l : listeners60)
            l->onAnimationFrame();

        if (++frameCount % 2 == 0)
        {
            for (auto* l : listeners30)
                l->onAnimationFrame();
        }
    }

    juce::Array<Listener*> listeners60;
    juce::Array<Listener*> listeners30;
    int frameCount = 0;
};
