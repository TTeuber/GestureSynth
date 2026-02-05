#include "MySynthVoice.h"

MySynthVoice::MySynthVoice (
    juce::AudioProcessorValueTreeState& p,
    juce::ValueTree& mt,
    std::shared_ptr<MyADSR*> ampEnvPtr,
    std::shared_ptr<PitchTracker> pt)
    : parameters (p),
      modTree (mt),
      env1ptr (std::move (ampEnvPtr)),
      pitchTracker (std::move (pt))
{
    modTree.addListener (this);

    parameters.addParameterListener ("filterOn", this);
    filterEnabled = *parameters.getRawParameterValue ("filterOn") > 0.5f;

    // Initialize the mod destinations
    // Currently this is how the current value is updated when there are no mod sources
    for (const auto& [_, d] : modDestinations)
        modMatrix.initDestination (d);

    for (auto* env : envs)
        env->setSampleRate (currentSampleRate);

    // lfo1.setFrequency (1.0f);
}

void MySynthVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, const int startSample, const int numSamples)
{
    juce::ScopedNoDenormals noDenormals;

    // Process any pending modulation commands from UI thread
    modMatrix.processPendingCommands();

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

    juneOscillator.processBlock (block);
    if (filterEnabled)
    {
        filter.setCutoffFrequency (filterCutoff.getCurrentValue());
        filter.setResonance (juce::jmax<float> (0.01, filterResonance.getCurrentValue()));
        filter.process (context);
    }
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float targetEnvVal = adsr1.getNextValue();

        modMatrix.processSample();

        if (currentEnvVal < targetEnvVal)
            currentEnvVal = juce::jmin (currentEnvVal + slewRate, targetEnvVal);
        else if (currentEnvVal > targetEnvVal)
            currentEnvVal = juce::jmax (currentEnvVal - slewRate, targetEnvVal);
        tempDataL[sample] *= currentEnvVal * velocity * volume;
        tempDataR[sample] *= currentEnvVal * velocity * volume;
    }

    outputBuffer.addFrom (0, startSample, tempBuffer, 0, 0, numSamples);
    outputBuffer.addFrom (1, startSample, tempBuffer, 1, 0, numSamples);

    waveformBuffer.write (tempBuffer, numSamples);
}

int MySynthVoice::readWaveformData (float* dest, int maxSamples)
{
    return waveformBuffer.read (dest, maxSamples);
}

void MySynthVoice::addNodeToMatrix (const juce::ValueTree& childNode)
{
    ModSource* source = modSources[childNode.getProperty ("source").toString()];
    ModDestination* destination = modDestinations[childNode.getProperty ("destination").toString()];
    if (source == nullptr || destination == nullptr)
        return;
    const float depth = childNode.getProperty ("depth");
    const bool isBipolar = childNode.getProperty ("isBipolar");
    modMatrix.queueAddModulation (destination, source, depth, isBipolar);
}
float MySynthVoice::frequencyToPhaseIncrement (const float frequency) const
{
    return 2.0f * juce::MathConstants<float>::pi * frequency / currentSampleRate;
}

void MySynthVoice::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == "filterOn")
    {
        const bool wasEnabled = filterEnabled;
        filterEnabled = newValue > 0.5f;

        // Reset filter state when re-enabled to prevent clicks/pops
        if (filterEnabled && !wasEnabled)
            filter.reset();
    }
}

void MySynthVoice::valueTreeChildAdded (juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded)
{
    addNodeToMatrix (childWhichHasBeenAdded);
}
void MySynthVoice::valueTreeChildRemoved (juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved)
{
    ModSource* source = modSources[childWhichHasBeenRemoved.getProperty ("source").toString()];
    ModDestination* destination = modDestinations[childWhichHasBeenRemoved.getProperty ("destination").toString()];
    if (source == nullptr || destination == nullptr)
        return;
    modMatrix.queueRemoveModulation (source, destination);
}
void MySynthVoice::valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property)
{
    const juce::StringRef sourceId = treeWhosePropertyHasChanged.getProperty ("source").toString();
    const juce::StringRef destinationId = treeWhosePropertyHasChanged.getProperty ("destination").toString();
    const float depth = treeWhosePropertyHasChanged.getProperty ("depth");

    auto* const source = modSources[sourceId];
    auto* const destination = modDestinations[destinationId];
    if (source == nullptr || destination == nullptr)
        return;

    if (property.toString() == "depth")
    {
        modMatrix.queueUpdateModulation (source, destination, depth);
    }
    else if (property.toString() == "source" || property.toString() == "destination")
    {
        modMatrix.queueRemoveModulation (source, destination);
        modMatrix.queueAddModulation (destination, source, depth, false);
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
    filter.prepare (spec);
    modMatrix.prepare (spec);
    waveformBuffer.reset();
}

void MySynthVoice::startNote (const int midiNoteNumber, const float velocity, juce::SynthesiserSound*, int currentPitchWheelPosition)
{
    frequency = static_cast<float> (juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber));
    juneOscillator.setFrequency (frequency);
    this->velocity = juce::jlimit (0.0f, 1.0f, velocity);
    for (auto* env : envs)
        env->noteOn();
    *env1ptr = &adsr1;
    pitchTracker->updateFrequency (frequency);
    waveLength = static_cast<int> (std::ceil (2 * currentSampleRate / frequency));
    while (waveLength > WaveformBuffer::kBufferSize)
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
