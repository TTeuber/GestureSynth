#include "MySynthVoice.h"
#include "../Utility/AtomicHelpers.h"

MySynthVoice::MySynthVoice (
    juce::AudioProcessorValueTreeState& p,
    juce::ValueTree& mt,
    std::array<std::shared_ptr<MyADSR*>, 4> envPtrsIn,
    std::shared_ptr<PitchTracker> pt,
    std::array<std::shared_ptr<LFOData>, 4>& lfoData,
    std::atomic<float>* velocityRawOut,
    std::atomic<float>* keyboardRawOut,
    std::atomic<float>* modWheelRawOut,
    std::atomic<float>* pitchBendRawOut,
    std::atomic<float>* aftertouchRawOut,
    std::atomic<float>* expressionRawOut)
    : parameters (p),
      modTree (mt),
      envPtrs (std::move (envPtrsIn)),
      pitchTracker (std::move (pt)),
      velocityRawOutput (velocityRawOut),
      keyboardRawOutput (keyboardRawOut),
      modWheelRawOutput (modWheelRawOut),
      pitchBendRawOutput (pitchBendRawOut),
      aftertouchRawOutput (aftertouchRawOut),
      expressionRawOutput (expressionRawOut)
{
    for (int i = 0; i < 4; ++i)
        if (lfoData[i])
            lfos[i].setLFOData (lfoData[i]);
    modTree.addListener (this);

    parameters.addParameterListener (ParamIDs::filterOn, this);
    filterEnabled = *parameters.getRawParameterValue (ParamIDs::filterOn) > 0.5f;

    parameters.addParameterListener (ParamIDs::hpfOn, this);
    hpfEnabled = *parameters.getRawParameterValue (ParamIDs::hpfOn) > 0.5f;


    registerModDestinations();
    for (const auto& [_, d] : modDestinations)
        modMatrix.initDestination (d);

    // Initialize slotCache from modTree
    for (int i = 0; i < modTree.getNumChildren() && i < static_cast<int> (slotCache.size()); ++i)
    {
        auto child = modTree.getChild (i);
        slotCache[i] = { child.getProperty ("source").toString(),
                         child.getProperty ("destination").toString() };
    }

    for (int i = 0; i < modTree.getNumChildren() && i < static_cast<int> (slotCache.size()); ++i)
        addNodeToMatrix (modTree.getChild (i));

    for (auto* env : envs)
        env->setSampleRate (currentSampleRate);
}

MySynthVoice::~MySynthVoice()
{
    modTree.removeListener (this);
    parameters.removeParameterListener (ParamIDs::filterOn, this);
    parameters.removeParameterListener (ParamIDs::hpfOn, this);
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

    if (frequency != targetFrequency)
    {
        if (portamentoTimeMs > 0.0f)
        {
            float portSamples = portamentoTimeMs * currentSampleRate / 1000.0f;
            float alpha = 1.0f - std::exp (-static_cast<float> (numSamples) / (portSamples * 0.2f));
            float logFreq = std::log2 (frequency);
            float logTarget = std::log2 (targetFrequency);
            logFreq += (logTarget - logFreq) * alpha;
            frequency = std::pow (2.0f, logFreq);
            if (std::abs (logFreq - logTarget) < 0.001f)
                frequency = targetFrequency;
        }
        else
        {
            frequency = targetFrequency;
        }
        pitchTracker->updateFrequency (frequency);
        waveLength = static_cast<int> (std::ceil (2 * currentSampleRate / frequency));
        while (waveLength > WaveformBuffer::kBufferSize)
            waveLength /= 2;
    }

    velocitySource.setCurve (velocityCurveParam.getValue());
    keyboardSource.setCurve (keyboardCurveParam.getValue());

    const float pitchBendRange = *parameters.getRawParameterValue (ParamIDs::pitchBendRange);
    const float pitchBendMultiplier = std::pow (2.0f, pitchBendRange * pitchBendValue / 12.0f);
    juneOscillator.setFrequency (frequency * vibrato.getFrequencyMultiplier (numSamples) * pitchBendMultiplier);
    juneOscillator.processBlock (block);

    if (noiseLevel > 0.0f)
    {
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float noiseSample = noiseRandom.nextFloat() * 2.0f - 1.0f;
            tempDataL[sample] += noiseSample * noiseLevel;
            tempDataR[sample] += noiseSample * noiseLevel;
        }
    }

    if (filterEnabled)
    {
        filter.setCutoffFrequency (filterCutoff.getCurrentValue());
        filter.setResonance (juce::jmax<float> (0.01, filterResonance.getCurrentValue()));
        filter.process (context);
    }

    if (hpfEnabled)
    {
        const float hpfFreq = hpfCutoff.getCurrentValue();
        for (auto& hpf : hpFilters)
        {
            hpf.setCutoffFrequency (hpfFreq);
            hpf.process (context);
        }
    }

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float targetEnvVal = adsr1.getNextValue();
        adsr2.getNextValue();
        adsr3.getNextValue();
        adsr4.getNextValue();

        modMatrix.processSample();

        vibrato.setDepth (vibratoDepthParam.getCurrentValue());
        vibrato.setRate (vibratoRateParam.getCurrentValue());

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
    if (static_cast<bool> (childNode.getProperty ("bypassed")))
        return;

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
    if (parameterID == ParamIDs::filterOn)
    {
        const bool wasEnabled = filterEnabled;
        filterEnabled = newValue > 0.5f;

        // Reset filter state when re-enabled to prevent clicks/pops
        if (filterEnabled && !wasEnabled)
            filter.reset();
    }
    else if (parameterID == ParamIDs::hpfOn)
    {
        const bool wasEnabled = hpfEnabled;
        hpfEnabled = newValue > 0.5f;

        if (hpfEnabled && !wasEnabled)
            for (auto& hpf : hpFilters)
                hpf.reset();
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
    else if (property.toString() == "bypassed")
    {
        const auto& [cachedSrc, cachedDst] = slotCache[slotIndex];
        auto dstIt = modDestinations.find (cachedDst);
        if (dstIt == modDestinations.end() || dstIt->second == nullptr)
            return;

        if (static_cast<bool> (treeWhosePropertyHasChanged.getProperty ("bypassed")))
        {
            modMatrix.queueRemoveModulation (slotIndex, dstIt->second);
        }
        else
        {
            addNodeToMatrix (treeWhosePropertyHasChanged);
        }
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
    for (auto& hpf : hpFilters)
    {
        hpf.prepare (spec);
        hpf.setType (juce::dsp::StateVariableTPTFilterType::highpass);
        hpf.setResonance (1.0f / std::sqrt (2.0f));
    }
    for (auto& lfo : lfos)
        lfo.prepare (spec);
    vibrato.prepare (static_cast<float> (sampleRate));
    modMatrix.prepare (spec);
    waveformBuffer.reset();
}

void MySynthVoice::startNote (const int midiNoteNumber, const float velocity, juce::SynthesiserSound*, int currentPitchWheelPosition)
{
    static uint64_t globalStartCounter = 0;
    voiceStartOrder = ++globalStartCounter;
    pitchBendValue = (currentPitchWheelPosition - 8192) / 8192.0f;
    targetFrequency = static_cast<float> (juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber));

    if (portamentoTimeMs > 0.0f && portamentoFromFrequency > 0.0f && !skipPortamento)
    {
        frequency = portamentoFromFrequency; // Glide from last played note
    }
    else
    {
        frequency = targetFrequency;
    }

    juneOscillator.setFrequency (frequency);
    vibrato.reset();
    this->velocity = juce::jlimit (0.0f, 1.0f, velocity);

    velocitySource.setValue (this->velocity);
    velocitySource.setCurve (velocityCurveParam.getValue());
    keyboardSource.setNoteNumber (midiNoteNumber);
    keyboardSource.setCurve (keyboardCurveParam.getValue());

    if (velocityRawOutput != nullptr)
        AtomicHelpers::paramStore (*velocityRawOutput, this->velocity);
    if (keyboardRawOutput != nullptr)
        AtomicHelpers::paramStore (*keyboardRawOutput, juce::jlimit (0.0f, 1.0f, static_cast<float> (midiNoteNumber) / 127.0f));

    for (auto* env : envs)
        env->noteOn();
    *envPtrs[0] = &adsr1;
    *envPtrs[1] = &adsr2;
    *envPtrs[2] = &adsr3;
    *envPtrs[3] = &adsr4;
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

void MySynthVoice::glideToNote (int midiNoteNumber, float vel)
{
    targetFrequency = static_cast<float> (juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber));
    // frequency stays as-is — voice glides from current position
    // Do NOT retrigger envelopes or reset vibrato
    velocity = juce::jlimit (0.0f, 1.0f, vel);
    keyboardSource.setNoteNumber (midiNoteNumber);
    keyboardSource.setCurve (keyboardCurveParam.getValue());
    pitchTracker->updateFrequency (targetFrequency);
    waveLength = static_cast<int> (std::ceil (2 * currentSampleRate / targetFrequency));
    while (waveLength > WaveformBuffer::kBufferSize)
        waveLength /= 2;
}
