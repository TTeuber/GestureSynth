#pragma once

#include "Modulation.h"
#include "MyParameter.h"

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
            const float attackTimeSeconds,
            const float decayTimeSeconds,
            const float newSustainLevel,
            const float releaseTimeSeconds,
            const float attackCurve = 1.0f,
            const float decayCurve = 1.0f,
            const float releaseCurve = 1.0f
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
        juce::StringRef attackId, decayId, sustainId, releaseId;
        float attack = 0.0f, decay = 0.5f, sustain = 1.0f, release = 0.0f;
        juce::StringRef attackCurveId, decayCurveId, releaseCurveId;
        float attackExponent = 1.0f, decayExponent = 1.0f, releaseExponent = 1.0f;
    };

    // MyADSR (juce::StringRef i, juce::StringRef n) : ModSource (i, n) {}
    // MyADSR (juce::StringRef i, juce::StringRef n, const Parameters& parameters)
    //     : ModSource (i, n),
    //       attackTime (parameters.attack),
    //       decayTime (parameters.decay),
    //       sustainLevel (parameters.sustain),
    //       releaseTime (parameters.release),
    //       attackExponent (parameters.attackExponent),
    //       decayExponent (parameters.decayExponent),
    //       releaseExponent (parameters.releaseExponent)
    // {
    //     recalculateRates();
    // }
    MyADSR (const juce::AudioProcessorValueTreeState& p, const int i)
        : ModSource (juce::StringRef ("env" + std::to_string (i)), juce::StringRef ("Envelope " + std::to_string (i))),
          attackTime (p.getParameter ("env" + std::to_string (i) + "Attack")),
          decayTime (p.getParameter ("env" + std::to_string (i) + "Decay")),
          sustainLevel (p.getParameter ("env" + std::to_string (i) + "Sustain")),
          releaseTime (p.getParameter ("env" + std::to_string (i) + "Release")),
          attackExponent (p.getParameter ("env" + std::to_string (i) + "AttackCurve")),
          decayExponent (p.getParameter ("env" + std::to_string (i) + "DecayCurve")),
          releaseExponent (p.getParameter ("env" + std::to_string (i) + "ReleaseCurve"))
    {
        recalculateRates();
    }

    void setParameters (const Parameters& parameters);
    void noteOn();
    void noteOff();
    void reset();
    void goToNextState();
    void recalculateRates();
    [[nodiscard]] float getCurrentValue() const noexcept override { return envelopeValue; }
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
    static float toReleaseCurve (const float time, const float sustain, const float exponent)
    {
        return sustain * (std::pow (1 - time, 1 / exponent) + 1 - std::pow (time, exponent)) / 2;
    }

    [[nodiscard]] bool isActive() const { return state != State::Idle; }
    [[nodiscard]] std::array<float, 2> getTimePoint() const { return { time, envelopeValue }; }
    [[nodiscard]] float getAttackExponent() const { return attackExponent.getValue(); }
    [[nodiscard]] float getDecayExponent() const { return decayExponent.getValue(); }
    [[nodiscard]] float getReleaseExponent() const { return releaseExponent.getValue(); }

private:
    State state = State::Idle;
    StaticParameter attackTime, decayTime, sustainLevel, releaseTime;
    float tempSustain = sustainLevel.getValue();
    StaticParameter attackExponent, decayExponent, releaseExponent;
    float attackRate = 0.0f, decayRate = 0.0f, releaseRate = 0.0f;
    float envelopeValue = 0.0f;
    float time = 0.0f;
};
