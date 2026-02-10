#pragma once

#include "Modulation/LFOData.h"
#include "Modulation/Modulation.h"
#include "Processor/Chorus.h"
#include "Synthesizer/MySynth.h"
#include "Utility/Parameters.h"
#include "Utility/PitchTracker.h"
#include "Utility/TempoInfo.h"
#include <array>
#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#if (MSVC)
    #include "ipps.h"
#endif

class PluginProcessor final : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool getWaveformData (float* dest, int maxSamples);

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    juce::MidiBuffer& getMidiMessages() { return midiBuffer; }

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::MidiKeyboardState keyboardState;
    juce::AudioProcessorValueTreeState parameters { *this, nullptr, "parameters", createLayout() };

    juce::ValueTree modTree { "modTree" };

    juce::AbstractFifo waveFifo { 2048 };
    std::vector<float> waveData = std::vector<float> (2048, 0.0f);

    using ModList = std::vector<std::tuple<juce::String, float, juce::String, bool>>;
    ModList modList = {
        { "env1", 0.0f, "filterFrequency", false },
        { "lfo1", 0.0f, "filterFrequency", true },
        { "lfo1", 0.0f, "pulseWidth", true },
    };

    MySynth& getSynth() { return synth; }

    std::shared_ptr<PitchTracker> pitchTracker = std::make_shared<PitchTracker>();
    std::array<std::shared_ptr<LFOData>, 4> lfoData = {
        std::make_shared<LFOData>(),
        std::make_shared<LFOData>(),
        std::make_shared<LFOData>(),
        std::make_shared<LFOData>()
    };

private:
    juce::MidiBuffer midiBuffer;

    MySynth synth;
    JuneChorus chorus { parameters };

    double lastProcessingTimeMs;
    double maxAllowedProcessingTimeMs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
