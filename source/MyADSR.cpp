//
// Created by Tyler Teuber on 3/12/25.
//

#include "MyADSR.h"
void MyADSR::setParameters (const Parameters& parameters)
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

void MyADSR::noteOn()
{
    if (attackRate > 0.0f)
    {
        envelopeValue = 0.0f;
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

void MyADSR::noteOff()
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
}

void MyADSR::reset()
{
    state = State::Idle;
    envelopeValue = 0.0f;
    time = 0.0f;
}

void MyADSR::goToNextState()
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
        state = State::Idle;
    reset();
}

void MyADSR::recalculateRates()
{
    attackRate = attackTime > 0.0f ? 1.0f / (attackTime * sampleRate) : -1.0f;
    decayRate = decayTime > 0.0f ? (1.0f - sustainLevel) / (decayTime * sampleRate) : -1.0f;
    releaseRate = releaseTime > 0.0f ? sustainLevel / (releaseTime * sampleRate) : -1.0f;
}

float MyADSR::getNextValue() noexcept
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