//
// Created by Tyler Teuber on 3/22/25.
//
#pragma once
#include "Modulation.h"
#include "MyOscillator.h"
#include <juce_dsp/juce_dsp.h>

class MyLFO final : public MyOscillator, public ModSource
{
public:
    MyLFO (const juce::StringRef i, const juce::StringRef n, const float f, const Waveform wave = Triangle) : MyOscillator (wave), ModSource (i, n), wave (wave)
    {
        setFrequency (f);
    }
    MyLFO (const MyLFO& other) : MyOscillator (other.wave), ModSource (other), wave (other.wave) {}
    ~MyLFO() override = default;

    float getNextValue() noexcept override
    {
        auto val = processSample (0.0f);
        return val;
    }
    Waveform wave;
};
