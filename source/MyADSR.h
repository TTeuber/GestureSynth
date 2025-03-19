#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_graphics/juce_graphics.h>

class MyADSR
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
            float attackCurve,
            float decayCurve,
            float releaseCurve
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
        float attack = 0.1f, decay = 0.1f, sustain = 1.0f, release = 0.1f;
        float attackExponent = 3.0f, decayExponent = 3.0f, releaseExponent = 3.0f;
    };

    void setParameters (const Parameters& parameters)
    {
        attackTime = parameters.attack;
        decayTime = parameters.decay;
        sustainLevel = parameters.sustain;
        releaseTime = parameters.release;
        attackExponent = parameters.attackExponent;
        decayExponent = parameters.decayExponent;
        releaseExponent = parameters.releaseExponent;

        recalculateRates();
    }

    void noteOn()
    {
        if (attackRate > 0.0f)
        {
            state = State::Attack;
        }
        else if (decayRate > 0.0f)
        {
            envelopeValue = 1.0f;
            state = State::Decay;
        }
        else
        {
            envelopeValue = sustainLevel;
            state = State::Sustain;
        }
        time = 0.0f;
    }

    void noteOff()
    {
        if (state != State::Idle)
        {
            tempSustain = envelopeValue;
            time = (attackTime + decayTime);
            if (releaseTime > 0.0f)
            {
                releaseRate = envelopeValue / (releaseTime * sampleRate);
                state = State::Release;
            }
            else
            {
                reset();
            }
        }
        state = State::Release;
    }

    void reset()
    {
        state = State::Idle;
        envelopeValue = 0.0f;
        time = 0.0f;
    }

    void goToNextState()
    {
        if (state == State::Attack)
        {
            state = decayRate > 0.0f ? State::Decay : State::Sustain;
            return;
        }

        if (state == State::Decay)
        {
            state = State::Sustain;
            return;
        }

        if (state == State::Release)
            reset();
    }

    void recalculateRates()
    {
        attackRate = attackTime > 0.0f ? 1.0f / (attackTime * sampleRate) : -1.0f;
        decayRate = decayTime > 0.0f ? (1.0f - sustainLevel) / (decayTime * sampleRate) : -1.0f;
        releaseRate = releaseTime > 0.0f ? sustainLevel / (releaseTime * sampleRate) : -1.0f;

        if ((state == State::Attack && attackRate <= 0.0f)
            || (state == State::Decay && (decayRate <= 0.0f || envelopeValue <= sustainLevel))
            || (state == State::Release && releaseRate <= 0.0f))
        {
            goToNextState();
        }
    }

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

    float getNextSample()
    {
        switch (state)
        {
            case State::Attack:
                envelopeValue = toAttackCurve (time / attackTime, attackExponent); // Ease-in attack
                if (envelopeValue >= 1.0f || time >= attackTime)
                {
                    state = State::Decay;
                    envelopeValue = 1.0f;
                }
                time += 1.0f / sampleRate;
                break;

            case State::Decay:
                if (sustainLevel < 1 && time < decayTime + attackTime)
                    envelopeValue = toDecayCurve ((time - attackTime) / decayTime, sustainLevel, decayExponent); // Ease-out decay
                else
                {
                    state = State::Sustain;
                    envelopeValue = sustainLevel;
                }
                time += 1.0f / sampleRate;
                break;

            case State::Sustain:
                envelopeValue = sustainLevel;
                break;

            case State::Release:
                if (envelopeValue <= 0.0f || time >= attackTime + decayTime + releaseTime)
                    state = State::Idle;
                else
                    envelopeValue = toReleaseCurve ((time - attackTime - decayTime) / releaseTime, tempSustain, releaseExponent); // Ease-out release
                time += 1.0f / sampleRate;
                break;

            case State::Idle:
                envelopeValue = 0.0f;
                break;

            default:
                break;
        }
        return envelopeValue;
    }

    void setSampleRate (const double newSampleRate)
    {
        sampleRate = static_cast<float> (newSampleRate);
    }

    [[nodiscard]] bool isActive() const
    {
        return state != State::Idle;
    }

    [[nodiscard]] std::array<float, 2> getTimePoint()
    {
        return { time, envelopeValue };
    }

    [[nodiscard]] float getAttackExponent() const { return attackExponent; }
    [[nodiscard]] float getDecayExponent() const { return decayExponent; }
    [[nodiscard]] float getReleaseExponent() const { return releaseExponent; }

private:
    State state = State::Idle;
    float attackTime = 0.1f, decayTime = 0.1f, sustainLevel = 0.7f, releaseTime = 0.2f;
    float tempSustain = sustainLevel;
    float attackExponent = 3.0f, decayExponent = 3.0f, releaseExponent = 3.0f;
    float attackRate = 0.0f, decayRate = 0.0f, releaseRate = 0.0f;
    float envelopeValue = 0.0f;
    float sampleRate = 44100.0f;
    float time = 0.0f;
};
