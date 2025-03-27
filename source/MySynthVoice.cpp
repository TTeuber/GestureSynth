#include "MySynthVoice.h"

#include <utility>

float MySynthVoice::frequencyToPhaseIncrement (const float frequency) const
{
    return 2.0f * juce::MathConstants<float>::pi * frequency / currentSampleRate;
}

MySynthVoice::MySynthVoice (juce::AudioProcessorValueTreeState& p, std::shared_ptr<MyADSR*> ampEnvPtr, std::shared_ptr<MyADSR*> filterEnvPtr)
    : parameters (p), ampADSRPtr (std::move (ampEnvPtr)), filterADSRPtr (std::move (filterEnvPtr))
{
    lfo.setFrequency (4.0f);

    for (auto* env : envs)
        env->setSampleRate (currentSampleRate);

    modMatrix.addModulation (&fineTuneParam, lfo, 0.05f, true);
    modMatrix.addModulation (&filterCutoff, ampADSR, 1.0f, false);
    modMatrix.addModulation (&filterCutoff, lfo, 0.5f, false);
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
    frequency = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
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

    for (int sample = 0; sample < numSamples; ++sample)
    {
        modMatrix.processSample();
        const float targetEnvVal = ampADSR.getNextValue();

        osc.setFilterCutoff (filterCutoff.getCurrentValue());

        osc.setFrequency (frequency + fineTuneParam.getCurrentValue() * (frequency * std::pow (2, 1 / 12)));

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
