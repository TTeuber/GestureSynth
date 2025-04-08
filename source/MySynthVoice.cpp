#include "MySynthVoice.h"

#include "juce_graphics/fonts/harfbuzz/hb-cplusplus.hh"

#include <utility>

float MySynthVoice::frequencyToPhaseIncrement (const float frequency) const
{
    return 2.0f * juce::MathConstants<float>::pi * frequency / currentSampleRate;
}

MySynthVoice::MySynthVoice (juce::AudioProcessorValueTreeState& p, juce::ValueTree& mt, std::shared_ptr<MyADSR*> ampEnvPtr, std::shared_ptr<MyADSR*> filterEnvPtr)
    : parameters (p), modTree (mt), ampADSRPtr (std::move (ampEnvPtr)), filterADSRPtr (std::move (filterEnvPtr))
{
    modTree.addListener (this);

    for (auto* env : envs)
        env->setSampleRate (currentSampleRate);

    lfo.setFrequency (7.0f);
}

void MySynthVoice::addNodeToMatrix (const juce::ValueTree& childNode)
{
    std::shared_ptr<ModSource> source = modSources[childNode.getProperty ("source").toString()];
    ModDestination* destination = modDestinations[childNode.getProperty ("destination").toString()];
    float depth = childNode.getProperty ("depth");
    bool isBipolar = childNode.getProperty ("isBipolar");
    modMatrix.addModulation (destination, source, depth, isBipolar);
}

void MySynthVoice::valueTreeChildAdded (juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded)
{
    addNodeToMatrix (childWhichHasBeenAdded);
}

void MySynthVoice::valueTreeChildRemoved (juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved)
{
    std::shared_ptr<ModSource> source = modSources[childWhichHasBeenRemoved.getProperty ("source").toString()];
    ModDestination* destination = modDestinations[childWhichHasBeenRemoved.getProperty ("destination").toString()];
    modMatrix.removeModulation (source, destination);
}

void MySynthVoice::valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property)
{
    const juce::StringRef sourceId = treeWhosePropertyHasChanged.getProperty ("source").toString();
    const juce::StringRef destinationId = treeWhosePropertyHasChanged.getProperty ("destination").toString();
    const float depth = treeWhosePropertyHasChanged.getProperty ("depth");

    auto source = modSources[sourceId];
    auto destination = modDestinations[destinationId];
    if (property.toString() == "depth")
    {
        modMatrix.updateModulation (source, destination, depth);
    }
    else if (property.toString() == "source" || property.toString() == "destination")
    {
        modMatrix.removeModulation (source, destination);
        modMatrix.addModulation (destination, source, depth, false);
    }
}

bool MySynthVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<MySynthSound*> (sound) != nullptr;
}

void MySynthVoice::prepare (double sampleRate, int samplesPerBlock, int numChannels)
{
    juce::dsp::ProcessSpec spec {};
    currentSampleRate = static_cast<float> (sampleRate);
    for (auto* env : envs)
        env->setSampleRate (sampleRate);
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = numChannels;

    osc.prepare (spec);
    modMatrix.prepare (spec);
}
void MySynthVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int currentPitchWheelPosition)
{
    frequency = static_cast<float> (juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber));
    osc.setFrequency (static_cast<float> (juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber)), true);
    this->velocity = juce::jlimit (0.0f, 1.0f, velocity);
    for (auto* env : envs)
        env->noteOn();
    *ampADSRPtr = &ampADSR;
    *filterADSRPtr = &filterADSR;
}

void MySynthVoice::stopNote (float velocity, bool allowTailOff)
{
    if (allowTailOff)
    {
        for (auto* env : envs)
            env->noteOff();
    }
    else
    {
        for (auto* env : envs)
            env->reset();
        clearCurrentNote();
    }
}

void MySynthVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!ampADSR.isActive())
    {
        clearCurrentNote();
    }
    if (!isVoiceActive())
    {
        return;
    }

    juce::AudioBuffer<float> tempBuffer (2, numSamples);
    tempBuffer.clear();
    auto* tempDataL = tempBuffer.getWritePointer (0);
    auto* tempDataR = tempBuffer.getWritePointer (1);

    juce::dsp::AudioBlock<float> block (tempBuffer);
    const juce::dsp::ProcessContextReplacing context (block);

    osc.process (context);

    osc.setFilterCutoff (filterCutoff.getCurrentValue());

    osc.setFrequency (frequency + fineTuneParam.getCurrentValue() * (frequency * std::pow (2, 1 / 12)));

    for (int sample = 0; sample < numSamples; ++sample)
    {
        modMatrix.processSample();

        const float targetEnvVal = ampADSR.getNextValue();

        if (currentEnvVal < targetEnvVal)
            currentEnvVal = juce::jmin (currentEnvVal + slewRate, targetEnvVal);
        else if (currentEnvVal > targetEnvVal)
            currentEnvVal = juce::jmax (currentEnvVal - slewRate, targetEnvVal);
        tempDataL[sample] *= currentEnvVal * velocity * volume;
        tempDataR[sample] *= currentEnvVal * velocity * volume;
    }

    outputBuffer.addFrom (0, startSample, tempBuffer, 0, 0, numSamples);
    outputBuffer.addFrom (1, startSample, tempBuffer, 1, 0, numSamples);
}
