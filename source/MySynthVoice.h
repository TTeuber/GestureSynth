#pragma once

#include "CustomOscillator.h"
#include "Modulation.h"
#include "MyADSR.h"
#include "MyLFO.h"
#include "MyParameter.h"

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
    MySynthVoice (juce::AudioProcessorValueTreeState& p, juce::ValueTree& mt, std::shared_ptr<MyADSR*> ampEnvPtr);
    void addNodeToMatrix (const juce::ValueTree& childNode);

    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void valueTreeChildAdded (juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded) override;
    void valueTreeChildRemoved (juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override;
    void valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override;

    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void prepare (double sampleRate, int samplesPerBlock, int numChannels);

    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int currentPitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    [[nodiscard]] float frequencyToPhaseIncrement (float frequency) const;

    std::shared_ptr<MyADSR*> getEnvPtr() { return env1ptr; }

    [[nodiscard]] float getVolume() const { return volume; }
    void setVolume (const float newVolume)
    {
        volume = newVolume;
        osc.setVolume (newVolume);
    }
    void setFilterCutoff (const float newFilterCutoff)
    {
        filterCutoff.setBaseValue (newFilterCutoff);
        osc.setFilterCutoff (newFilterCutoff);
    }
    void setFilterResonance (const float newFilterResonance)
    {
        filterResonance.setBaseValue (newFilterResonance);
        osc.setFilterResonance (newFilterResonance);
    }

private:
    CustomOscillator osc;
    juce::AudioProcessorValueTreeState& parameters;
    juce::ValueTree& modTree;
    ModMatrix modMatrix;

    MyLFO lfo1 = MyLFO ("lfo1", "LFO 1", 1.0f);

    DynamicParameter fineTuneParam = DynamicParameter (parameters.getParameter ("fineTune"));

    float frequency = 0.0f;
    float currentSampleRate = 48000.0f;

    float currentEnvVal = 0.0f;
    float slewRate = 0.005f;

    float volume = 1.0f;
    float velocity = 0.0f;

    DynamicParameter filterCutoff = DynamicParameter (parameters.getParameter ("filterFrequency"));
    DynamicParameter filterResonance = DynamicParameter (parameters.getParameter ("filterResonance"));

    MyADSR adsr1 = MyADSR (parameters, 1);
    std::shared_ptr<MyADSR*> env1ptr;

    // MyADSR::Parameters env1Params = { 0.0f, 0.5f, 1.0f, 0.7f, 1, 1, 1 };

    std::vector<MyADSR*> envs = { &adsr1 };

    std::map<juce::String, std::shared_ptr<ModSource>> modSources = {
        { lfo1.getID(), std::shared_ptr<MyLFO> (&lfo1) },
        { adsr1.getID(), std::shared_ptr<MyADSR> (&adsr1) },
    };

    std::map<juce::String, ModDestination*> modDestinations = {
        { filterCutoff.getID(), &filterCutoff },
        { filterResonance.getID(), &filterResonance },
        { fineTuneParam.getID(), &fineTuneParam }
    };
};
