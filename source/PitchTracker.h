//
// Created by Tyler Teuber on 4/9/25.
//

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

class PitchTracker
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void update (float newFrequency) = 0;
        float frequency = 440.0f;
    };
    float frequency = 440.0f;

    void addListener (Listener* listener)
    {
        listeners.push_back (listener);
    }
    void updateFrequency (const float newFrequency)
    {
        frequency = newFrequency;
        for (const auto& listener : listeners)
        {
            listener->update (frequency);
        }
    }

private:
    std::vector<Listener*> listeners;
};
