#include "MySynthVoice.h"

MySynthVoice::MySynthVoice (
    juce::AudioProcessorValueTreeState& p,
    juce::ValueTree& mt,
    std::shared_ptr<MyADSR*> ampEnvPtr,
    std::shared_ptr<PitchTracker> pt,
    std::shared_ptr<MySynthVoice*> vp)
    : parameters (p),
      modTree (mt),
      voicePtr (std::move (vp)),
      env1ptr (std::move (ampEnvPtr)),
      pitchTracker (std::move (pt))
{
    modTree.addListener (this);

    for (auto* env : envs)
        env->setSampleRate (currentSampleRate);

    lfo1.setFrequency (7.0f);
}

void MySynthVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, const int startSample, const int numSamples)
{
    if (!adsr1.isActive())
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

    // osc.process (context);
    // juneOscillator.setPulseWidth (pulseWidth.getCurrentValue());
    juneOscillator.processBlock (block);

    // osc.setFilterCutoff (filterCutoff.getCurrentValue());

    // osc.setFrequency (static_cast<float> (frequency + fineTuneParam.getCurrentValue() * (frequency * std::pow (2, 1 / 12))));

    volume = parameters.getParameter ("volume")->getValue();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        modMatrix.processSample();

        const float targetEnvVal = adsr1.getNextValue();

        if (currentEnvVal < targetEnvVal)
            currentEnvVal = juce::jmin (currentEnvVal + slewRate, targetEnvVal);
        else if (currentEnvVal > targetEnvVal)
            currentEnvVal = juce::jmax (currentEnvVal - slewRate, targetEnvVal);
        tempDataL[sample] *= currentEnvVal * velocity * volume;
        tempDataR[sample] *= currentEnvVal * velocity * volume;
    }

    outputBuffer.addFrom (0, startSample, tempBuffer, 0, 0, numSamples);
    outputBuffer.addFrom (1, startSample, tempBuffer, 1, 0, numSamples);

    waveformBuffer.writeBuffer (outputBuffer, outputBuffer.getNumSamples());
}

const juce::AudioBuffer<float>& MySynthVoice::getWaveformBuffer()
{
    return waveformBuffer.getBuffer();
}

void MySynthVoice::addNodeToMatrix (const juce::ValueTree& childNode)
{
    const std::shared_ptr<ModSource> source = modSources[childNode.getProperty ("source").toString()];
    ModDestination* destination = modDestinations[childNode.getProperty ("destination").toString()];
    const float depth = childNode.getProperty ("depth");
    const bool isBipolar = childNode.getProperty ("isBipolar");
    modMatrix.addModulation (destination, source, depth, isBipolar);
}
float MySynthVoice::frequencyToPhaseIncrement (const float frequency) const
{
    return 2.0f * juce::MathConstants<float>::pi * frequency / currentSampleRate;
}

void MySynthVoice::parameterChanged (const juce::String& parameterID, float newValue)
{
    // for (auto& [id, source] : modSources)
    // {
    //     if (id == parameterID)
    //     {
    //         source->setValue (newValue);
    //         break;
    //     }
    // }
}

void MySynthVoice::valueTreeChildAdded (juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded)
{
    addNodeToMatrix (childWhichHasBeenAdded);
}
void MySynthVoice::valueTreeChildRemoved (juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved)
{
    const std::shared_ptr<ModSource> source = modSources[childWhichHasBeenRemoved.getProperty ("source").toString()];
    ModDestination* destination = modDestinations[childWhichHasBeenRemoved.getProperty ("destination").toString()];
    modMatrix.removeModulation (source, destination);
}
void MySynthVoice::valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property)
{
    const juce::StringRef sourceId = treeWhosePropertyHasChanged.getProperty ("source").toString();
    const juce::StringRef destinationId = treeWhosePropertyHasChanged.getProperty ("destination").toString();
    const float depth = treeWhosePropertyHasChanged.getProperty ("depth");

    const auto source = modSources[sourceId];
    const auto destination = modDestinations[destinationId];
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
void MySynthVoice::prepare (const double sampleRate, const int samplesPerBlock, const int numChannels)
{
    juce::dsp::ProcessSpec spec {};
    currentSampleRate = static_cast<float> (sampleRate);
    for (auto* env : envs)
        env->setSampleRate (sampleRate);
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = numChannels;

    juneOscillator.prepare (spec);
    osc.prepare (spec);
    modMatrix.prepare (spec);
}

void MySynthVoice::startNote (const int midiNoteNumber, const float velocity, juce::SynthesiserSound*, int currentPitchWheelPosition)
{
    frequency = static_cast<float> (juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber));
    osc.setFrequency (static_cast<float> (juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber)), true);
    juneOscillator.setFrequency (frequency);
    this->velocity = juce::jlimit (0.0f, 1.0f, velocity);
    for (auto* env : envs)
        env->noteOn();
    *env1ptr = &adsr1;
    *voicePtr = this;
    pitchTracker->updateFrequency (frequency);
    waveLength = static_cast<int> (std::ceil (2 * currentSampleRate / frequency));
    while (waveLength > waveformBuffer.getBuffer().getNumSamples())
        waveLength /= 2;
}
void MySynthVoice::stopNote (float velocity, const bool allowTailOff)
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
