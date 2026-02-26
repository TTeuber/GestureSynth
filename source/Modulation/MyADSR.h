#pragma once

#include "../Utility/MyParameter.h"
#include "../Utility/CurveUtils.h"
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
            const float attackTimeSeconds,
            const float decayTimeSeconds,
            const float newSustainLevel,
            const float releaseTimeSeconds,
            const float attackCurveParam = 0.0f,
            const float decayCurveParam = 0.0f,
            const float releaseCurveParam = 0.0f
            ):
            attack (attackTimeSeconds),
            decay (decayTimeSeconds),
            sustain (newSustainLevel),
            release (releaseTimeSeconds),
            attackCurve (attackCurveParam),
            decayCurve (decayCurveParam),
            releaseCurve (releaseCurveParam)
        // clang-format on
        {
        }
        juce::StringRef attackId, decayId, sustainId, releaseId;
        float attack = 0.0f, decay = 0.5f, sustain = 1.0f, release = 0.0f;
        juce::StringRef attackCurveId, decayCurveId, releaseCurveId;
        float attackCurve = 0.0f, decayCurve = 0.0f, releaseCurve = 0.0f;
    };

    MyADSR (const juce::AudioProcessorValueTreeState& p, const int i)
        : ModSource (juce::StringRef ("env" + std::to_string (i)), juce::StringRef ("Envelope " + std::to_string (i))),
          attackTime (p.getParameter ("env" + std::to_string (i) + "Attack")),
          decayTime (p.getParameter ("env" + std::to_string (i) + "Decay")),
          sustainLevel (p.getParameter ("env" + std::to_string (i) + "Sustain")),
          releaseTime (p.getParameter ("env" + std::to_string (i) + "Release")),
          attackCurve (p.getParameter ("env" + std::to_string (i) + "AttackCurve")),
          decayCurve (p.getParameter ("env" + std::to_string (i) + "DecayCurve")),
          releaseCurve (p.getParameter ("env" + std::to_string (i) + "ReleaseCurve"))
    {
        recalculateRates();
        attackTime.onUpdate ([this] { recalculateRates(); });
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

    static float toAttackCurve (const float time, const float k)
    {
        return CurveUtils::applyCurve (time, k);
    }
    static float toDecayCurve (const float time, const float sustain, const float k)
    {
        return sustain + (1.0f - sustain) * (1.0f - CurveUtils::applyCurve (time, k));
    }
    static float toReleaseCurve (const float time, const float sustain, const float k)
    {
        jassert (time <= 1.0f && time >= 0.0f);
        const float val = sustain * (1.0f - CurveUtils::applyCurve (time, k));
        jassert (!std::isnan (val));
        return val;
    }

    [[nodiscard]] bool isActive() const { return state != State::Idle; }
    [[nodiscard]] std::array<float, 2> getTimePoint() const { return { time, envelopeValue }; }
    [[nodiscard]] float getAttackCurve() const { return attackCurve.getValue(); }
    [[nodiscard]] float getDecayCurve() const { return decayCurve.getValue(); }
    [[nodiscard]] float getReleaseCurve() const { return releaseCurve.getValue(); }

private:
    State state = State::Idle;
    StaticParameter attackTime, decayTime, sustainLevel, releaseTime;
    float tempSustain = sustainLevel.getValue();
    StaticParameter attackCurve, decayCurve, releaseCurve;
    float attackRate = 0.0f, decayRate = 0.0f, releaseRate = 0.0f;
    float envelopeValue = 0.0f;
    float time = 0.0f;
};
