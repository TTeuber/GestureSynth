//
// Created by Tyler Teuber on 3/22/25.
//
#pragma once
#include "Modulation.h"
#include "MyOscillator.h"
#include <juce_dsp/juce_dsp.h>

class MyLFO : public MyOscillator, public ModSource
{
public:
    explicit MyLFO (const Waveform wave = Sine) : MyOscillator (wave) {}

    float getNextValue() noexcept override
    {
        return this->processSample (0.0f);
    }
};
