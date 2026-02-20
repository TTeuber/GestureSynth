#include "MySynthVoice.h"

MySynthVoice::MySynthVoice (
    juce::AudioProcessorValueTreeState& p,
    juce::ValueTree& mt,
    std::shared_ptr<MyADSR*> ampEnvPtr,
    std::shared_ptr<PitchTracker> pt,
    std::array<std::shared_ptr<LFOData>, 4>& lfoData)
    : parameters (p),
      modTree (mt),
      env1ptr (std::move (ampEnvPtr)),
      pitchTracker (std::move (pt))
{
    for (int i = 0; i < 4; ++i)
        if (lfoData[i])
            lfos[i].setLFOData (lfoData[i]);
    modTree.addListener (this);

    parameters.addParameterListener ("filterOn", this);
    filterEnabled = *parameters.getRawParameterValue ("filterOn") > 0.5f;

    // Initialize the mod destinations
    // Currently this is how the current value is updated when there are no mod sources
    for (const auto& [_, d] : modDestinations)
        modMatrix.initDestination (d);

    // Initialize slotCache from modTree
    for (int i = 0; i < modTree.getNumChildren() && i < static_cast<int> (slotCache.size()); ++i)
    {
        auto child = modTree.getChild (i);
        slotCache[i] = { child.getProperty ("source").toString(),
                         child.getProperty ("destination").toString() };
    }

    for (auto* env : envs)
        env->setSampleRate (currentSampleRate);
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

    juneOscillator.setFrequency (frequency * vibrato.getFrequencyMultiplier (numSamples));
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
        adsr2.getNextValue();
        adsr3.getNextValue();
        adsr4.getNextValue();

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
    auto srcIt = modSources.find (childNode.getProperty ("source").toString());
    auto dstIt = modDestinations.find (childNode.getProperty ("destination").toString());
    if (srcIt == modSources.end() || dstIt == modDestinations.end())
        return;
    auto* source = srcIt->second;
    auto* destination = dstIt->second;
    if (source == nullptr || destination == nullptr)
        return;
    const float depth = childNode.getProperty ("depth");
    const bool isBipolar = childNode.getProperty ("isBipolar");
    const int slotIndex = childNode.getParent().indexOf (childNode);
    modMatrix.queueAddModulation (destination, source, depth, isBipolar, slotIndex);
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
    const int slotIndex = parentTree.indexOf (childWhichHasBeenAdded);
    if (slotIndex >= 0 && slotIndex < static_cast<int> (slotCache.size()))
    {
        slotCache[slotIndex] = {
            childWhichHasBeenAdded.getProperty ("source").toString(),
            childWhichHasBeenAdded.getProperty ("destination").toString()
        };
    }
    addNodeToMatrix (childWhichHasBeenAdded);
}
void MySynthVoice::valueTreeChildRemoved (juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved)
{
    auto dstIt = modDestinations.find (childWhichHasBeenRemoved.getProperty ("destination").toString());
    if (dstIt == modDestinations.end())
        return;
    auto* destination = dstIt->second;
    if (destination == nullptr)
        return;
    modMatrix.queueRemoveModulation (indexFromWhichChildWasRemoved, destination);
}
void MySynthVoice::valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property)
{
    // Find which slot index this child belongs to
    auto parent = treeWhosePropertyHasChanged.getParent();
    if (!parent.isValid())
        return;
    const int slotIndex = parent.indexOf (treeWhosePropertyHasChanged);
    if (slotIndex < 0 || slotIndex >= static_cast<int> (slotCache.size()))
        return;

    if (property.toString() == "depth")
    {
        // Use cached source/dest (they haven't changed)
        const auto& [cachedSrc, cachedDst] = slotCache[slotIndex];
        auto srcIt = modSources.find (cachedSrc);
        auto dstIt = modDestinations.find (cachedDst);
        if (srcIt == modSources.end() || dstIt == modDestinations.end())
            return;
        auto* source = srcIt->second;
        auto* destination = dstIt->second;
        if (source == nullptr || destination == nullptr)
            return;
        const float depth = treeWhosePropertyHasChanged.getProperty ("depth");
        modMatrix.queueUpdateModulation (slotIndex, destination, depth);
    }
    else if (property.toString() == "source" || property.toString() == "destination")
    {
        // Remove OLD routing using cached values
        const auto& [oldSrc, oldDst] = slotCache[slotIndex];
        auto oldSrcIt = modSources.find (oldSrc);
        auto oldDstIt = modDestinations.find (oldDst);
        if (oldSrcIt != modSources.end() && oldDstIt != modDestinations.end()
            && oldSrcIt->second != nullptr && oldDstIt->second != nullptr)
        {
            modMatrix.queueRemoveModulation (slotIndex, oldDstIt->second);
        }

        // Update cache with new values
        const juce::String newSrc = treeWhosePropertyHasChanged.getProperty ("source").toString();
        const juce::String newDst = treeWhosePropertyHasChanged.getProperty ("destination").toString();
        slotCache[slotIndex] = { newSrc, newDst };

        // Add NEW routing
        auto newSrcIt = modSources.find (newSrc);
        auto newDstIt = modDestinations.find (newDst);
        if (newSrcIt == modSources.end() || newDstIt == modDestinations.end())
            return;
        auto* source = newSrcIt->second;
        auto* destination = newDstIt->second;
        if (source == nullptr || destination == nullptr)
            return;
        const float depth = treeWhosePropertyHasChanged.getProperty ("depth");
        const bool isBipolar = treeWhosePropertyHasChanged.getProperty ("isBipolar");
        modMatrix.queueAddModulation (destination, source, depth, isBipolar, slotIndex);
    }
    else if (property.toString() == "isBipolar")
    {
        // Update cache source/dest haven't changed, but we need to re-add with new bipolar setting
        const auto& [cachedSrc, cachedDst] = slotCache[slotIndex];
        auto srcIt = modSources.find (cachedSrc);
        auto dstIt = modDestinations.find (cachedDst);
        if (srcIt == modSources.end() || dstIt == modDestinations.end())
            return;
        auto* source = srcIt->second;
        auto* destination = dstIt->second;
        if (source == nullptr || destination == nullptr)
            return;
        modMatrix.queueRemoveModulation (slotIndex, destination);
        const float depth = treeWhosePropertyHasChanged.getProperty ("depth");
        const bool isBipolar = treeWhosePropertyHasChanged.getProperty ("isBipolar");
        modMatrix.queueAddModulation (destination, source, depth, isBipolar, slotIndex);
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
    for (auto& lfo : lfos)
        lfo.prepare (spec);
    vibrato.prepare (static_cast<float> (sampleRate));
    modMatrix.prepare (spec);
    waveformBuffer.reset();
}

void MySynthVoice::startNote (const int midiNoteNumber, const float velocity, juce::SynthesiserSound*, int currentPitchWheelPosition)
{
    frequency = static_cast<float> (juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber));
    juneOscillator.setFrequency (frequency);
    vibrato.reset();
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
