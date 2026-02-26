#pragma once

#include <cmath>

namespace CurveUtils
{
    /**
     * Applies a curve to a normalized value x in [0, 1].
     * k in [-1, 1]: k=0 is linear, k>0 bows upward, k<0 bows downward.
     * Formula: f(x) = x / (x + 1000^(-k) * (1 - x))
     */
    inline float applyCurve (float x, float k) noexcept
    {
        if (x <= 0.0f) return 0.0f;
        if (x >= 1.0f) return 1.0f;
        float w = std::pow (1000.0f, -k);
        return x / (x + w * (1.0f - x));
    }

    /**
     * Inverse of applyCurve at x=0.5: given the output y at the midpoint,
     * returns the curve parameter k.
     * Formula: k = -ln((1/y - 1)) / ln(1000)
     */
    inline float inverseCurveAtHalf (float y) noexcept
    {
        y = std::fmax (0.001f, std::fmin (0.999f, y));
        return -std::log ((1.0f / y) - 1.0f) / std::log (1000.0f);
    }
}
