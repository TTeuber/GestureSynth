#pragma once

#include "MySynthVoice.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class MySynth final : public juce::Synthesiser
{
public:
    explicit MySynth (juce::AudioProcessorValueTreeState& p);
    static void updateParameter (float& currentValue, float newValue, const std::function<void (float)>& setterFunction);

    float getVolume() const { return masterVolume; }

    float getFilterCutoff() const { return filterCutoff; }

    float getFilterEnvelopeAmount() const { return filterEnvelopeAmount; }

    float getFilterResonance() const { return filterResonance; }

    void updateParameters();
    template <class Func>
    void applyToAllVoices (Func&& function);

    std::shared_ptr<MyADSR*> getAmpADSRPtr() { return ampEnvPtr; }
    std::shared_ptr<MyADSR*> getFilterADSRPtr() { return filterEnvPtr; }

private:
    juce::AudioProcessorValueTreeState& parameters;

    float masterVolume = 0.0f;
    float filterCutoff = 0.0f;
    float filterEnvelopeAmount = 0.0f;
    float filterResonance = 0.0f;

    float ampAttack = 0.0f;
    float ampDecay = 0.0f;
    float ampSustain = 0.0f;
    float ampRelease = 0.0f;

    float filterAttack = 0.0f;
    float filterDecay = 0.0f;
    float filterSustain = 0.0f;
    float filterRelease = 0.0f;

    std::shared_ptr<MyADSR*> ampEnvPtr = std::make_shared<MyADSR*>();
    std::shared_ptr<MyADSR*> filterEnvPtr = std::make_shared<MyADSR*>();
};
