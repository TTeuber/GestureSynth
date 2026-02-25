//
// VelocitySource.h
//
#pragma once

#include "Modulation.h"
#include <cmath>

class VelocitySource final : public ModSource
{
public:
    VelocitySource() : ModSource ("velocity", "Velocity") {}

    void setValue (float v) noexcept { rawValue = v; }
    void setCurve (float c) noexcept { curve = c; }

    float getNextValue() noexcept override { return applyCurve (rawValue, curve); }
    [[nodiscard]] float getCurrentValue() const noexcept override { return applyCurve (rawValue, curve); }

    static float applyCurve (float input, float c) noexcept
    {
        if (input <= 0.0f) return 0.0f;
        if (input >= 1.0f) return 1.0f;
        float exponent = std::pow (2.0f, -c * 3.0f);
        return std::pow (input, exponent);
    }

private:
    float rawValue = 0.0f;
    float curve = 0.0f;
};
