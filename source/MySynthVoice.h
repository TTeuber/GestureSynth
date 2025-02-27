#pragma once

#include "CustomOscillator.h"
#include <juce_dsp/juce_dsp.h>

class MySynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int midiNoteNumber) override { return true; }
    bool appliesToChannel (int /*midiChannel*/) override { return true; }
};

class MySynthVoice : public juce::SynthesiserVoice
{
public:
    MySynthVoice();
    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void prepare (double sampleRate, int samplesPerBlock, int numChannels);

    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int currentPitchWheelPosition) override;

    void stopNote (float velocity, bool allowTailOff) override;

    void pitchWheelMoved (int) override {};
    void controllerMoved (int, int) override {};

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    [[nodiscard]] float frequencyToPhaseIncrement (float frequency) const;

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

private:
    CustomOscillator osc;

    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    float level = 0.0f;
    float tailOff = 0.0f;
    float frequency = 0.0f;
    float currentSampleRate = 48000.0f;

    float currentGain = 0.0f;
    float targetGain = 0.0f;
    float slewRate = 0.005f;

    float volume = 1.0f;

    float filterCutoff = 0.0f;
    bool filterEnvelopeActive = true;
    float filterEnvelopeAmount = 0.0f;
    float filterResonance = 0.0f;

    juce::ADSR ampADSR;
    juce::ADSR filterADSR;

    juce::ADSR::Parameters ampEnvParams;
    juce::ADSR::Parameters filterEnvParams;
};
