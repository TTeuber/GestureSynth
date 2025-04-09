#pragma once

#include "Modulation.h"
#include "MySynth.h"
#include "juce_dsp/juce_dsp.h"
#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>

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

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout() const;

    juce::MidiKeyboardState keyboardState;
    juce::AudioProcessorValueTreeState parameters { *this, nullptr, "parameters", createLayout() };

    juce::ValueTree modTree { "modTree" };

    using ModList = std::vector<std::tuple<juce::String, float, juce::String, bool>>;
    ModList modList = {
        // { "filterADSR", 0.5f, "filterFrequency", false },
        // { "lfo1", 0.5f, "fineTune", true },
        // { "ampADSR", 0.2f, "filterFrequency", false },
        // { "lfo1", 0.1, "fineTune", false }
    };

    std::vector<std::tuple<juce::String, juce::String>> envs = {
        { "adsr1", "Envelope 1" },
        // { "adsr2", "Envelope 2" },
        // { "adsr3", "Envelope 3" },
        // { "adsr4", "Envelope 4" }
    };

    MySynth& getSynth() { return synth; }

private:
    juce::MidiBuffer midiBuffer;

    MySynth synth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
