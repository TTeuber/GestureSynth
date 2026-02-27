#pragma once

#include "../Modulation/LFOData.h"
#include "../Utility/PitchTracker.h"
#include "../Utility/TempoInfo.h"
#include "../Utility/TempoSyncUtils.h"
#include "MySynthVoice.h"

#include <array>
#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
class MySynth final : public juce::Synthesiser
{
public:
    explicit MySynth (juce::AudioProcessorValueTreeState& p, juce::ValueTree& mt, std::shared_ptr<PitchTracker> pt, std::array<std::shared_ptr<LFOData>, 4>& lfoData);
    static void updateParameter (float& currentValue, float newValue, const std::function<void (float)>& setterFunction);

    void noteOn (int midiChannel, int midiNoteNumber, float velocity) override;
    void noteOff (int midiChannel, int midiNoteNumber, float velocity, bool allowTailOff) override;

    float getVolume() const { return masterVolume; }

    float getFilterCutoff() const { return filterCutoff; }

    float getFilterEnvelopeAmount() const { return filterEnvelopeAmount; }

    float getFilterResonance() const { return filterResonance; }

    void updateParameters (const TempoInfo& tempoInfo);
    template <class Func>
    void applyToAllVoices (Func&& function);

    void setSourceOutputArray (std::atomic<float>* arr);
    void setDestOutputArray (std::atomic<float>* arr);

    void resetOutputsIfIdle();

    std::shared_ptr<MyADSR*> getAmpADSRPtr() { return ampEnvPtr; }
    std::shared_ptr<MyADSR*> getFilterADSRPtr() { return filterEnvPtr; }

    std::atomic<float>* getVelocityRawPtr() { return &currentVelocityRaw; }
    std::atomic<float>* getKeyboardRawPtr() { return &currentKeyboardRaw; }
    std::atomic<float>* getModWheelRawPtr() { return &currentModWheelRaw; }
    std::atomic<float>* getPitchBendRawPtr() { return &currentPitchBendRaw; }

private:
    juce::AudioProcessorValueTreeState& parameters;
    juce::ValueTree& modTree;

    float masterVolume = 0.0f;
    float noiseLevel = 0.0f;
    float filterCutoff = 0.0f;
    float filterEnvelopeAmount = 0.0f;
    float filterResonance = 0.0f;

    float ampAttack = 0.0f;
    float ampAttackCurve = 1.0f;
    float ampDecay = 0.0f;
    float ampDecayCurve = 1.0f;
    float ampSustain = 0.0f;
    float ampRelease = 0.0f;
    float ampReleaseCurve = 1.0f;

    float filterAttack = 0.0f;
    float filterAttackCurve = 1.0f;
    float filterDecay = 0.0f;
    float filterDecayCurve = 1.0f;
    float filterSustain = 0.0f;
    float filterRelease = 0.0f;
    float filterReleaseCurve = 1.0f;

    std::array<float, 4> lfoRates = { 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<bool, 4> lfoTempoSync = { false, false, false, false };
    std::array<int, 4> lfoNoteDivision = { 6, 6, 6, 6 };
    std::array<bool, 4> lfoBeatSync = { false, false, false, false };
    std::array<bool, 4> lfoMono = { false, false, false, false };
    float manualBpm = 120.0f;

    float portamentoTime = 0.0f;

    int lastMidiNote = -1;
    float lastNoteFrequency = 0.0f;
    std::vector<int> heldNotes;
    MySynthVoice* monoVoice = nullptr;
    bool monoMode = false;
    bool legatoMode = false;

    bool vibratoOn = false;

    std::shared_ptr<MyADSR*> ampEnvPtr = std::make_shared<MyADSR*>();
    std::shared_ptr<MyADSR*> filterEnvPtr = std::make_shared<MyADSR*>();

    std::atomic<float> currentVelocityRaw { 0.0f };
    std::atomic<float> currentKeyboardRaw { 0.0f };
    std::atomic<float> currentModWheelRaw { 0.0f };
    std::atomic<float> currentPitchBendRaw { 8192.0f };
};
