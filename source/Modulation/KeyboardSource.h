//
// KeyboardSource.h
//
#pragma once

#include "VelocitySource.h"

class KeyboardSource final : public ModSource
{
public:
    KeyboardSource() : ModSource ("keyboard", "Keyboard") {}

    void setNoteNumber (int midiNote) noexcept
    {
        rawValue = juce::jlimit (0.0f, 1.0f, static_cast<float> (midiNote) / 127.0f);
    }

    void setCurve (float c) noexcept { curve = c; }

    float getNextValue() noexcept override { return VelocitySource::applyCurve (rawValue, curve); }
    [[nodiscard]] float getCurrentValue() const noexcept override { return VelocitySource::applyCurve (rawValue, curve); }

private:
    float rawValue = 0.0f;
    float curve = 0.0f;
};
