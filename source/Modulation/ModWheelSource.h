//
// ModWheelSource.h
//
#pragma once

#include "Modulation.h"

class ModWheelSource final : public ModSource
{
public:
    ModWheelSource() : ModSource ("modWheel", "Mod Wheel") {}

    void setValue (float v) noexcept { value = v; }

    float getNextValue() noexcept override { return value; }
    [[nodiscard]] float getCurrentValue() const noexcept override { return value; }

private:
    float value = 0.0f;
};
