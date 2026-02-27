//
// AftertouchSource.h
//
#pragma once

#include "Modulation.h"

class AftertouchSource final : public ModSource
{
public:
    AftertouchSource() : ModSource ("aftertouch", "Aftertouch") {}

    void setValue (float v) noexcept { value = v; }

    float getNextValue() noexcept override { return value; }
    [[nodiscard]] float getCurrentValue() const noexcept override { return value; }

private:
    float value = 0.0f;
};
