#pragma once

#include "MySynth.h"
#include "juce_dsp/juce_dsp.h"
#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>

#if (MSVC)
    #include "ipps.h"
#endif

class PluginProcessor : public juce::AudioProcessor
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

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::MidiKeyboardState keyboardState;
    juce::AudioProcessorValueTreeState parameters { *this, nullptr, "parameters", createLayout() };

    MySynth& getSynth() { return synth; }

    void tick() const
    {
        for (auto& func : functions)
            func();
    }

    void onTick (const std::function<void()>& func)
    {
        functions.push_back (func);
    }

private:
    juce::MidiBuffer midiBuffer;

    MySynth synth;

    std::vector<std::function<void()>> functions = {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
