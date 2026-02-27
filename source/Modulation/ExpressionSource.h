//
// ExpressionSource.h
//
#pragma once

#include "Modulation.h"

class ExpressionSource final : public ModSource
{
public:
    ExpressionSource() : ModSource ("expression", "Expression") {}

    void setValue (float v) noexcept { value = v; }

    float getNextValue() noexcept override { return value; }
    [[nodiscard]] float getCurrentValue() const noexcept override { return value; }

private:
    float value = 0.0f;
};
