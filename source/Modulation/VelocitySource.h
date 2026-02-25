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

    static float applyCurve (float x, float k) noexcept
    {
        if (x <= 0.0f) return 0.0f;
        if (x >= 1.0f) return 1.0f;
        float w = std::pow (1000.0f, -k);
        return x / (x + w * (1.0f - x));
    }

private:
    float rawValue = 0.0f;
    float curve = 0.0f;
};
