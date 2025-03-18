#pragma once

#include "CustomOscillator.h"
#include "MyADSR.h"
#include <juce_dsp/juce_dsp.h>

class MySynth;
class MySynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int midiNoteNumber) override { return true; }
    bool appliesToChannel (int /*midiChannel*/) override { return true; }
};

class MySynthVoice : public juce::SynthesiserVoice
{
public:
    MySynthVoice (std::shared_ptr<MyADSR*> ampEnvPtr, std::shared_ptr<MyADSR*> filterEnvPtr);

    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void prepare (double sampleRate, int samplesPerBlock, int numChannels);

    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int currentPitchWheelPosition) override;

    void stopNote (float velocity, bool allowTailOff) override;

    void pitchWheelMoved (int) override {};
    void controllerMoved (int, int) override {};

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    [[nodiscard]] float frequencyToPhaseIncrement (float frequency) const;

    std::shared_ptr<MyADSR*> getAmpPtr() { return ampADSRPtr; }
    std::shared_ptr<MyADSR*> getFilterPtr() { return filterADSRPtr; }

    [[nodiscard]] float getVolume() const { return volume; }

    void setVolume (const float newVolume)
    {
        volume = newVolume;
        osc.setVolume (newVolume);
    }

    void setFilterCutoff (const float newFilterCutoff)
    {
        filterCutoff = newFilterCutoff;
        if (!filterEnvelopeActive)
            osc.setFilterCutoff (newFilterCutoff);
    }

    void setFilterEnvelopeAmount (const float newFilterEnvelopeAmount)
    {
        filterEnvelopeAmount = newFilterEnvelopeAmount;
    }

    void setFilterResonance (const float newFilterResonance)
    {
        filterResonance = newFilterResonance;
        osc.setFilterResonance (newFilterResonance);
    }

    void setAmpAttack (const float newAttack)
    {
        ampEnvParams.attack = newAttack;
        ampADSR.setParameters (ampEnvParams);
    }

    void setAmpAttackCurve (const float newCurve)
    {
        ampEnvParams.attackExponent = newCurve;
        ampADSR.setParameters (ampEnvParams);
    }

    void setAmpDecay (const float newDecay)
    {
        ampEnvParams.decay = newDecay;
        ampADSR.setParameters (ampEnvParams);
    }

    void setAmpDecayCurve (const float newCurve)
    {
        ampEnvParams.decayExponent = newCurve;
        ampADSR.setParameters (ampEnvParams);
    }

    void setAmpSustain (const float newSustain)
    {
        ampEnvParams.sustain = newSustain;
        ampADSR.setParameters (ampEnvParams);
    }

    void setAmpRelease (const float newRelease)
    {
        ampEnvParams.release = newRelease;
        ampADSR.setParameters (ampEnvParams);
    }

    void setAmpReleaseCurve (const float newCurve)
    {
        ampEnvParams.releaseExponent = newCurve;
        ampADSR.setParameters (ampEnvParams);
    }

    void setFilterAttack (const float newAttack)
    {
        filterEnvParams.attack = newAttack;
        filterADSR.setParameters (filterEnvParams);
    }

    void setFilterAttackCurve (const float newCurve)
    {
        filterEnvParams.attackExponent = newCurve;
        filterADSR.setParameters (filterEnvParams);
    }

    void setFilterDecay (const float newDecay)
    {
        filterEnvParams.decay = newDecay;
        filterADSR.setParameters (filterEnvParams);
    }

    void setFilterDecayCurve (const float newCurve)
    {
        filterEnvParams.decayExponent = newCurve;
        filterADSR.setParameters (filterEnvParams);
    }

    void setFilterSustain (const float newSustain)
    {
        filterEnvParams.sustain = newSustain;
        filterADSR.setParameters (filterEnvParams);
    }

    void setFilterRelease (const float newRelease)
    {
        filterEnvParams.release = newRelease;
        filterADSR.setParameters (filterEnvParams);
    }

    void setFilterReleaseCurve (const float newCurve)
    {
        filterEnvParams.releaseExponent = newCurve;
        filterADSR.setParameters (filterEnvParams);
    }

private:
    CustomOscillator osc;

    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    float level = 0.0f;
    float tailOff = 0.0f;
    float frequency = 0.0f;
    float currentSampleRate = 48000.0f;

    float currentEnvVal = 0.0f;
    float slewRate = 0.005f;

    float volume = 1.0f;
    float velocity = 0.0f;

    float filterCutoff = 0.0f;
    bool filterEnvelopeActive = true;
    float filterEnvelopeAmount = 0.0f;
    float filterResonance = 0.0f;

    MyADSR ampADSR;
    std::shared_ptr<MyADSR*> ampADSRPtr;

    MyADSR filterADSR;
    std::shared_ptr<MyADSR*> filterADSRPtr;

    MyADSR::Parameters ampEnvParams = { 0.1f, 0.5f, 0.2f, 0.7f };
    MyADSR::Parameters filterEnvParams = { 0.1f, 0.5f, 0.0f, 0.5f };
};
