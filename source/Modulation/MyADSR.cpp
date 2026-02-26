//
// Created by Tyler Teuber on 3/12/25.
//

#include "MyADSR.h"
void MyADSR::setParameters (const Parameters& parameters)
{
    attackTime.setValue (parameters.attack);
    decayTime.setValue (parameters.decay);
    sustainLevel.setValue (parameters.sustain);
    releaseTime.setValue (parameters.release);
    attackCurve.setValue (parameters.attackCurve);
    decayCurve.setValue (parameters.decayCurve);
    releaseCurve.setValue (parameters.releaseCurve);

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
        envelopeValue = sustainLevel.getValue();
        state = State::Sustain;
    }
    time = 0.0f;
}
void MyADSR::noteOff()
{
    if (state != State::Idle)
    {
        tempSustain = envelopeValue;
        time = attackTime.getValue() + decayTime.getValue();
        if (releaseTime.getValue() > 0.0f)
        {
            releaseRate = envelopeValue / (releaseTime.getValue() * sampleRate);
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
    attackRate = attackTime.getValue() > 0.0f ? 1.0f / (attackTime.getValue() * sampleRate) : -1.0f;
    decayRate = decayTime.getValue() > 0.0f ? (1.0f - sustainLevel.getValue()) / (decayTime.getValue() * sampleRate) : -1.0f;
    releaseRate = releaseTime.getValue() > 0.0f ? sustainLevel.getValue() / (releaseTime.getValue() * sampleRate) : -1.0f;
}

float MyADSR::getNextValue() noexcept
{
    switch (state)
    {
        case State::Attack:
            envelopeValue = toAttackCurve (time / attackTime.getValue(), attackCurve.getValue()); // Ease-in attack
            if (envelopeValue >= 1.0f || time >= attackTime.getValue())
            {
                state = State::Decay;
                envelopeValue = 1.0f;
            }
            time += 1.0f / sampleRate;
            break;

        case State::Decay:
            if (sustainLevel.getValue() < 1 && time < decayTime.getValue() + attackTime.getValue())
                envelopeValue = toDecayCurve (juce::jlimit<float> (0, 1, (time - attackTime.getValue()) / decayTime.getValue()), sustainLevel.getValue(), decayCurve.getValue()); // Ease-out decay
            else
            {
                state = State::Sustain;
                envelopeValue = sustainLevel.getValue();
            }
            time += 1.0f / sampleRate;
            break;

        case State::Sustain:
            envelopeValue = sustainLevel.getValue();
            break;

        case State::Release:
            if (envelopeValue <= 0.0f || time >= attackTime.getValue() + decayTime.getValue() + releaseTime.getValue())
                state = State::Idle;
            else
                envelopeValue = toReleaseCurve (juce::jlimit<float> (0, 1, (time - attackTime.getValue() - decayTime.getValue()) / releaseTime.getValue()), tempSustain, releaseCurve.getValue()); // Ease-out release
            time += 1.0f / sampleRate;
            break;

        case State::Idle:
            envelopeValue = 0.0f;
            break;

        default:
            break;
    }
    jassert (!std::isnan (envelopeValue));
    return envelopeValue;
}