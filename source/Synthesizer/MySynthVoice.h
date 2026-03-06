#pragma once

#include "../Utility/AtomicHelpers.h"
#include "../Modulation/KeyboardSource.h"
#include "../Modulation/AftertouchSource.h"
#include "../Modulation/ExpressionSource.h"
#include "../Modulation/ModWheelSource.h"
#include "../Modulation/VelocitySource.h"
#include "../Modulation/Modulation.h"
#include "../Modulation/MyADSR.h"
#include "../Modulation/MyLFO.h"
#include "../Processor/JuneOscillator.h"
#include "../Processor/Vibrato.h"
#include "../Utility/MyParameter.h"
#include "../Utility/Parameters.h"
#include "../Utility/PitchTracker.h"

#include <array>
#include <juce_dsp/juce_dsp.h>

class MySynth;
class MySynthSound final : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int /*midiNoteNumber*/) override { return true; }
    bool appliesToChannel (int /*midiChannel*/) override { return true; }
};

class MySynthVoice final : public juce::SynthesiserVoice, public juce::ValueTree::Listener, juce::AudioProcessorValueTreeState::Listener
{
public:
    MySynthVoice (juce::AudioProcessorValueTreeState& p, juce::ValueTree& mt, std::shared_ptr<MyADSR*> ampEnvPtr, std::shared_ptr<PitchTracker> pt, std::array<std::shared_ptr<LFOData>, 4>& lfoData,
                  std::atomic<float>* velocityRawOut = nullptr, std::atomic<float>* keyboardRawOut = nullptr,
                  std::atomic<float>* modWheelRawOut = nullptr, std::atomic<float>* pitchBendRawOut = nullptr,
                  std::atomic<float>* aftertouchRawOut = nullptr, std::atomic<float>* expressionRawOut = nullptr);
    void addNodeToMatrix (const juce::ValueTree& childNode);

    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void valueTreeChildAdded (juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded) override;
    void valueTreeChildRemoved (juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override;
    void valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override;

    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void prepare (double sampleRate, int samplesPerBlock, int numChannels);

    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int currentPitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;

    void pitchWheelMoved (int newPitchWheelValue) override
    {
        pitchBendValue = (newPitchWheelValue - 8192) / 8192.0f;
        if (pitchBendRawOutput != nullptr)
            AtomicHelpers::paramStore (*pitchBendRawOutput, static_cast<float> (newPitchWheelValue));
    }

    void controllerMoved (int controllerNumber, int newControllerValue) override
    {
        if (controllerNumber == 1)
        {
            modWheelSource.setValue (newControllerValue / 127.0f);
            if (modWheelRawOutput != nullptr)
                AtomicHelpers::paramStore (*modWheelRawOutput, newControllerValue / 127.0f);
        }
        else if (controllerNumber == 11)
        {
            expressionSource.setValue (newControllerValue / 127.0f);
            if (expressionRawOutput != nullptr)
                AtomicHelpers::paramStore (*expressionRawOutput, newControllerValue / 127.0f);
        }
    }

    void aftertouchChanged (int newAftertouchValue) override
    {
        aftertouchSource.setValue (newAftertouchValue / 127.0f);
        if (aftertouchRawOutput != nullptr)
            AtomicHelpers::paramStore (*aftertouchRawOutput, newAftertouchValue / 127.0f);
    }

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;
    int readWaveformData (float* dest, int maxSamples);

    [[nodiscard]] float frequencyToPhaseIncrement (float frequency) const;

    std::shared_ptr<MyADSR*> getEnvPtr() { return env1ptr; }

    float getOversamplingLatency() const { return juneOscillator.getLatencyInSamples(); }

    [[nodiscard]] float getVolume() const { return volume; }
    void setVolume (const float newVolume)
    {
        volume = newVolume;
    }
    void setFilterCutoff (const float newFilterCutoff)
    {
        filterCutoff.setBaseValue (newFilterCutoff);
    }
    void setFilterResonance (const float newFilterResonance)
    {
        filterResonance.setBaseValue (newFilterResonance);
    }
    void setLFORate (int index, float rate) { lfos[index].setRate (rate); }
    void setLFOPhase (int index, float phase) { lfos[index].setPhase (phase); }
    float getLFOPhase (int index) const { return lfos[index].getPhase(); }
    uint64_t getVoiceStartOrder() const { return voiceStartOrder; }

    void setNoiseLevel (float level) { noiseLevel = level; }

    void setPortamentoTime (float timeMs) { portamentoTimeMs = timeMs; }

    void setPortamentoStart (float freq, bool skip)
    {
        portamentoFromFrequency = freq;
        skipPortamento = skip;
    }

    void glideToNote (int midiNoteNumber, float vel);

    void setSourceOutputArray (std::atomic<float>* arr) { modMatrix.setSourceOutputArray (arr); }
    void setDestOutputArray (std::atomic<float>* arr) { modMatrix.setDestOutputArray (arr); }

    void resetModOutputs() { modMatrix.resetOutputs(); }

    void setVibratoEnabled (bool enabled) { vibrato.setEnabled (enabled); }

    [[nodiscard]] int getWavelength() const
    {
        return waveLength;
    }

private:
    juce::AudioProcessorValueTreeState& parameters;
    juce::ValueTree& modTree;
    ModMatrix modMatrix;

    DynamicParameter pulseWidth = DynamicParameter (parameters.getParameter (ParamIDs::pulseWidth));
    DynamicParameter oscWaveform = DynamicParameter (parameters.getParameter (ParamIDs::oscWaveform));
    DynamicParameter oscDetune = DynamicParameter (parameters.getParameter (ParamIDs::oscDetune));
    DynamicParameter oscWidth = DynamicParameter (parameters.getParameter (ParamIDs::oscWidth));
    DynamicParameter subOsc = DynamicParameter (parameters.getParameter (ParamIDs::subOsc));
    DynamicParameter subOscWave = DynamicParameter (parameters.getParameter (ParamIDs::subOscWave));
    JuneDCO juneOscillator = JuneDCO (parameters, pulseWidth, oscWaveform, oscDetune, oscWidth, subOsc, subOscWave);

    std::array<MyLFO, 4> lfos = {
        MyLFO { "lfo1", "LFO 1", 1.0f },
        MyLFO { "lfo2", "LFO 2", 1.0f },
        MyLFO { "lfo3", "LFO 3", 1.0f },
        MyLFO { "lfo4", "LFO 4", 1.0f }
    };

    DynamicParameter vibratoDepthParam = DynamicParameter (parameters.getParameter (ParamIDs::vibratoDepth));
    DynamicParameter vibratoRateParam = DynamicParameter (parameters.getParameter (ParamIDs::vibratoRate));
    DynamicParameter chorusDepthParam = DynamicParameter (parameters.getParameter (ParamIDs::chorusDepth));
    DynamicParameter chorusRateParam = DynamicParameter (parameters.getParameter (ParamIDs::chorusRate));

    DynamicParameter fineTuneParam = DynamicParameter (parameters.getParameter (ParamIDs::fineTune));

    juce::dsp::StateVariableTPTFilter<float> filter = juce::dsp::StateVariableTPTFilter<float>();
    std::array<juce::dsp::StateVariableTPTFilter<float>, 2> hpFilters;

    ModWheelSource modWheelSource;
    AftertouchSource aftertouchSource;
    ExpressionSource expressionSource;
    VelocitySource velocitySource;
    KeyboardSource keyboardSource;
    StaticParameter velocityCurveParam = StaticParameter (parameters.getParameter (ParamIDs::velocityCurve));
    StaticParameter keyboardCurveParam = StaticParameter (parameters.getParameter (ParamIDs::keyboardCurve));
    float pitchBendValue = 0.0f;

    float frequency = 0.0f;
    float targetFrequency = 0.0f;
    float portamentoTimeMs = 0.0f;
    float portamentoFromFrequency = 0.0f;
    bool skipPortamento = false;
    float currentSampleRate = 48000.0f;
    Vibrato vibrato;

    float currentEnvVal = 0.0f;
    float slewRate = 0.005f;

    float volume = 1.0f;
    float noiseLevel = 0.0f;
    juce::Random noiseRandom;
    float velocity = 0.0f;

    DynamicParameter filterCutoff = DynamicParameter (parameters.getParameter (ParamIDs::filterFrequency));
    DynamicParameter filterResonance = DynamicParameter (parameters.getParameter (ParamIDs::filterResonance));
    bool filterEnabled = false;

    DynamicParameter hpfCutoff = DynamicParameter (parameters.getParameter (ParamIDs::hpfFrequency));
    bool hpfEnabled = false;
    bool gateMode = false;
    float gateAmp = 0.0f;
    bool gateReleasing = false;

    MyADSR adsr1 = MyADSR (parameters, 1);
    MyADSR adsr2 = MyADSR (parameters, 2);
    MyADSR adsr3 = MyADSR (parameters, 3);
    MyADSR adsr4 = MyADSR (parameters, 4);
    std::shared_ptr<MyADSR*> env1ptr;

    std::shared_ptr<PitchTracker> pitchTracker;

    struct WaveformBuffer
    {
        static constexpr int kBufferSize = 2048;

        WaveformBuffer() : fifo (kBufferSize)
        {
            buffer.setSize (1, kBufferSize);
            buffer.clear();
        }

        void write (const juce::AudioBuffer<float>& inputBuffer, int numSamples)
        {
            const int samplesToWrite = juce::jmin (numSamples, fifo.getFreeSpace());
            if (samplesToWrite == 0)
                return;

            const auto scope = fifo.write (samplesToWrite);
            if (scope.blockSize1 > 0)
                for (int i = 0; i < scope.blockSize1; ++i)
                    buffer.setSample (0, scope.startIndex1 + i, inputBuffer.getSample (0, i));
            if (scope.blockSize2 > 0)
                for (int i = 0; i < scope.blockSize2; ++i)
                    buffer.setSample (0, scope.startIndex2 + i, inputBuffer.getSample (0, scope.blockSize1 + i));
        }

        int read (float* dest, int maxSamples)
        {
            const int numReady = fifo.getNumReady();
            if (numReady == 0)
                return 0;

            const int samplesToRead = juce::jmin (numReady, maxSamples);
            const auto scope = fifo.read (samplesToRead);
            if (scope.blockSize1 > 0)
                for (int i = 0; i < scope.blockSize1; ++i)
                    dest[i] = buffer.getSample (0, scope.startIndex1 + i);
            if (scope.blockSize2 > 0)
                for (int i = 0; i < scope.blockSize2; ++i)
                    dest[scope.blockSize1 + i] = buffer.getSample (0, scope.startIndex2 + i);
            return samplesToRead;
        }

        int getNumReady() const { return fifo.getNumReady(); }
        void reset() { fifo.reset(); }

    private:
        juce::AbstractFifo fifo;
        juce::AudioBuffer<float> buffer;
    };
    WaveformBuffer waveformBuffer;
    uint64_t voiceStartOrder = 0;
    double startTime = 0.0;
    int waveLength = 1024;

    std::vector<MyADSR*> envs = { &adsr1, &adsr2, &adsr3, &adsr4 };

    std::map<juce::String, ModSource*> modSources = {
        { lfos[0].getID(), &lfos[0] },
        { lfos[1].getID(), &lfos[1] },
        { lfos[2].getID(), &lfos[2] },
        { lfos[3].getID(), &lfos[3] },
        { adsr1.getID(), &adsr1 },
        { adsr2.getID(), &adsr2 },
        { adsr3.getID(), &adsr3 },
        { adsr4.getID(), &adsr4 },
        { modWheelSource.getID(), &modWheelSource },
        { aftertouchSource.getID(), &aftertouchSource },
        { expressionSource.getID(), &expressionSource },
        { velocitySource.getID(), &velocitySource },
        { keyboardSource.getID(), &keyboardSource },
    };

    std::map<juce::String, ModDestination*> modDestinations = {
        { filterCutoff.getID(), &filterCutoff },
        { filterResonance.getID(), &filterResonance },
        { hpfCutoff.getID(), &hpfCutoff },
        { fineTuneParam.getID(), &fineTuneParam },
        { pulseWidth.getID(), &pulseWidth },
        { oscWaveform.getID(), &oscWaveform },
        { oscDetune.getID(), &oscDetune },
        { oscWidth.getID(), &oscWidth },
        { subOsc.getID(), &subOsc },
        { subOscWave.getID(), &subOscWave },
        { vibratoDepthParam.getID(), &vibratoDepthParam },
        { vibratoRateParam.getID(), &vibratoRateParam },
        { chorusDepthParam.getID(), &chorusDepthParam },
        { chorusRateParam.getID(), &chorusRateParam }
    };

    std::atomic<float>* velocityRawOutput = nullptr;
    std::atomic<float>* keyboardRawOutput = nullptr;
    std::atomic<float>* modWheelRawOutput = nullptr;
    std::atomic<float>* pitchBendRawOutput = nullptr;
    std::atomic<float>* aftertouchRawOutput = nullptr;
    std::atomic<float>* expressionRawOutput = nullptr;

    // Cache of source/dest per slot for correct removal on property change
    std::array<std::pair<juce::String, juce::String>, 16> slotCache;
};
