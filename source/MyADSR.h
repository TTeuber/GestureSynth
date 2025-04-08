#pragma once

#include "Modulation.h"

#include <juce_dsp/juce_dsp.h>
#include <juce_graphics/juce_graphics.h>

class MyADSR final : public ModSource
{
public:
    enum class State {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release
    };

    struct Parameters
    {
        Parameters() = default;

        // clang-format off
        Parameters (
            float attackTimeSeconds,
            float decayTimeSeconds,
            float newSustainLevel,
            float releaseTimeSeconds,
            float attackCurve = 1.0f,
            float decayCurve = 1.0f,
            float releaseCurve = 1.0f
            ):
            attack (attackTimeSeconds),
            decay (decayTimeSeconds),
            sustain (newSustainLevel),
            release (releaseTimeSeconds),
            attackExponent (attackCurve),
            decayExponent (decayCurve),
            releaseExponent (releaseCurve)
        // clang-format on
        {
        }
        float attack = 0.0f, decay = 0.5f, sustain = 1.0f, release = 0.0f;
        float attackExponent = 1.0f, decayExponent = 1.0f, releaseExponent = 1.0f;
    };

    MyADSR (juce::StringRef i, juce::StringRef n) : ModSource (i, n) {}
    MyADSR (juce::StringRef i, juce::StringRef n, const Parameters& parameters)
        : ModSource (i, n),
          attackTime (parameters.attack),
          decayTime (parameters.decay),
          sustainLevel (parameters.sustain),
          releaseTime (parameters.release),
          attackExponent (parameters.attackExponent),
          decayExponent (parameters.decayExponent),
          releaseExponent (parameters.releaseExponent)
    {
        recalculateRates();
    }

    void setParameters (const Parameters& parameters);
    void noteOn();
    void noteOff();
    void reset();
    void goToNextState();
    void recalculateRates();
    float getCurrentValue() const noexcept { return envelopeValue; }
    float getNextValue() noexcept override;
    void setSampleRate (const double newSampleRate) { sampleRate = static_cast<float> (newSampleRate); }

    static float toAttackCurve (const float time, const float exponent)
    {
        return (1 - std::pow (1 - time, 1 / exponent) + std::pow (time, exponent)) / 2;
    }

    static float toDecayCurve (const float time, const float sustain, const float exponent)
    {
        return sustain + (1 - sustain) * (std::pow (1 - time, 1 / exponent) + 1 - std::pow (time, exponent)) / 2;
    }

    static float toReleaseCurve (const float time, const float sustain, float exponent)
    {
        return sustain * (std::pow (1 - time, 1 / exponent) + 1 - std::pow (time, exponent)) / 2;
    }

    [[nodiscard]] bool isActive() const { return state != State::Idle; }
    [[nodiscard]] std::array<float, 2> getTimePoint() const { return { time, envelopeValue }; }
    [[nodiscard]] float getAttackExponent() const { return attackExponent; }
    [[nodiscard]] float getDecayExponent() const { return decayExponent; }
    [[nodiscard]] float getReleaseExponent() const { return releaseExponent; }

private:
    State state = State::Idle;
    float attackTime = 0.0f, decayTime = 0.5f, sustainLevel = 1.0f, releaseTime = 0.0f;
    float tempSustain = sustainLevel;
    float attackExponent = 3.0f, decayExponent = 3.0f, releaseExponent = 3.0f;
    float attackRate = 0.0f, decayRate = 0.0f, releaseRate = 0.0f;
    float envelopeValue = 0.0f;
    // float sampleRate = 44100.0f;
    float time = 0.0f;
};
