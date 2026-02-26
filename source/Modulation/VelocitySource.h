//
// VelocitySource.h
//
#pragma once

#include "Modulation.h"
#include "../Utility/CurveUtils.h"

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
        return CurveUtils::applyCurve (x, k);
    }

private:
    float rawValue = 0.0f;
    float curve = 0.0f;
};
