#pragma once

#include "../Modulation/LFOData.h"
#include "../Utility/Parameters.h"
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

    void noteOn (int midiChannel, int midiNoteNumber, float velocity) override;
    void noteOff (int midiChannel, int midiNoteNumber, float velocity, bool allowTailOff) override;

    void updateParameters (const TempoInfo& tempoInfo);
    void setVoiceCount (int count);
    void prepareVoices (double sampleRate, int samplesPerBlock, int numChannels);
    template <class Func>
    void applyToAllVoices (Func&& function);

    void setSourceOutputArray (std::atomic<float>* arr);
    void setDestOutputArray (std::atomic<float>* arr);

    void resetOutputsIfIdle();

    std::shared_ptr<MyADSR*> getAmpADSRPtr() { return ampEnvPtr; }
    std::shared_ptr<MyADSR*> getFilterADSRPtr() { return filterEnvPtr; }

    std::shared_ptr<MyADSR*> getEnvPtr (int index)
    {
        jassert (index >= 0 && index < 4);
        return envPtrs[index];
    }

    std::shared_ptr<MyLFO*> getLFOPtr (int index)
    {
        jassert (index >= 0 && index < 4);
        return lfoPtrs[index];
    }

    std::atomic<float>* getVelocityRawPtr() { return &currentVelocityRaw; }
    std::atomic<float>* getKeyboardRawPtr() { return &currentKeyboardRaw; }
    std::atomic<float>* getModWheelRawPtr() { return &currentModWheelRaw; }
    std::atomic<float>* getPitchBendRawPtr() { return &currentPitchBendRaw; }
    std::atomic<float>* getAftertouchRawPtr() { return &currentAftertouchRaw; }
    std::atomic<float>* getExpressionRawPtr() { return &currentExpressionRaw; }

private:
    juce::AudioProcessorValueTreeState& parameters;
    juce::ValueTree& modTree;

    // APVTS atomic pointers — cached once, read directly from APVTS atomics
    std::atomic<float>* volumeParam           = nullptr;
    std::atomic<float>* noiseOnParam          = nullptr;
    std::atomic<float>* filterCutoffParam     = nullptr;
    std::atomic<float>* filterResonanceParam  = nullptr;
    std::atomic<float>* portamentoTimeParam   = nullptr;
    std::atomic<float>* vibratoOnParam        = nullptr;
    std::atomic<float>* monoOnParam           = nullptr;
    std::atomic<float>* legatoOnParam         = nullptr;
    std::atomic<float>* voiceCountParam       = nullptr;
    std::atomic<float>* manualBpmParam        = nullptr;

    struct LFOParamPtrs
    {
        std::atomic<float>* rate          = nullptr;
        std::atomic<float>* tempoSync     = nullptr;
        std::atomic<float>* noteDivision  = nullptr;
        std::atomic<float>* beatSync      = nullptr;
        std::atomic<float>* mono          = nullptr;
    };
    std::array<LFOParamPtrs, 4> lfoParams {};

    // Previous-value cache for change detection (only stores last-applied values)
    float prevVolume          = -1.0f;
    bool  prevNoiseOn         = false;
    float prevFilterCutoff    = -1.0f;
    float prevFilterResonance = -1.0f;
    float prevPortamentoTime  = -1.0f;
    bool  prevVibratoOn       = false;
    std::array<float, 4> prevLfoRates = { -1.0f, -1.0f, -1.0f, -1.0f };

    int currentVoiceCount = 8;

    double preparedSampleRate = 0.0;
    int preparedSamplesPerBlock = 0;
    int preparedNumChannels = 0;

    int lastMidiNote = -1;
    float lastNoteFrequency = 0.0f;
    std::vector<int> heldNotes;
    MySynthVoice* monoVoice = nullptr;
    bool monoMode = false;
    bool legatoMode = false;

    std::shared_ptr<MyADSR*> ampEnvPtr = std::make_shared<MyADSR*>();
    std::shared_ptr<MyADSR*> filterEnvPtr = std::make_shared<MyADSR*>();
    std::shared_ptr<MyADSR*> env3Ptr = std::make_shared<MyADSR*>();
    std::shared_ptr<MyADSR*> env4Ptr = std::make_shared<MyADSR*>();
    std::array<std::shared_ptr<MyADSR*>, 4> envPtrs = { ampEnvPtr, filterEnvPtr, env3Ptr, env4Ptr };

    std::shared_ptr<MyLFO*> lfoPtr1 = std::make_shared<MyLFO*> (nullptr);
    std::shared_ptr<MyLFO*> lfoPtr2 = std::make_shared<MyLFO*> (nullptr);
    std::shared_ptr<MyLFO*> lfoPtr3 = std::make_shared<MyLFO*> (nullptr);
    std::shared_ptr<MyLFO*> lfoPtr4 = std::make_shared<MyLFO*> (nullptr);
    std::array<std::shared_ptr<MyLFO*>, 4> lfoPtrs = { lfoPtr1, lfoPtr2, lfoPtr3, lfoPtr4 };

    std::shared_ptr<PitchTracker> pitchTracker;
    std::array<std::shared_ptr<LFOData>, 4>* lfoDataPtr = nullptr;

    std::atomic<float> currentVelocityRaw { 0.0f };
    std::atomic<float> currentKeyboardRaw { 0.0f };
    std::atomic<float> currentModWheelRaw { 0.0f };
    std::atomic<float> currentPitchBendRaw { 8192.0f };
    std::atomic<float> currentAftertouchRaw { 0.0f };
    std::atomic<float> currentExpressionRaw { 0.0f };

    std::atomic<float>* cachedSourceOutputArray = nullptr;
    std::atomic<float>* cachedDestOutputArray = nullptr;
};
