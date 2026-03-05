#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Synthesizer/MySynthVoice.h"

//==============================================================================
PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties()
#if !JucePlugin_IsMidiEffect
    #if !JucePlugin_IsSynth
              .withInput ("Input", juce::AudioChannelSet::stereo(), true)
    #endif
              .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
              ),
      synth (parameters, modTree, pitchTracker, lfoData),
      lastProcessingTimeMs (0.0),
      maxAllowedProcessingTimeMs (0.0)
{
    for (size_t i = 0; i < modList.size(); i++)
    {
        juce::ValueTree childNode ("modulation" + juce::String (static_cast<int> (i)));
        childNode.setProperty ("source", std::get<0> (modList[i]), nullptr);
        childNode.setProperty ("depth", std::get<1> (modList[i]), nullptr);
        childNode.setProperty ("destination", std::get<2> (modList[i]), nullptr);
        childNode.setProperty ("isBipolar", std::get<3> (modList[i]), nullptr);
        childNode.setProperty ("bypassed", false, nullptr);
        modTree.appendChild (childNode, nullptr);
    }

    synth.setSourceOutputArray (modSourceOutputs.data());
    synth.setDestOutputArray (modDestOutputs.data());
}

PluginProcessor::~PluginProcessor() = default;

//==============================================================================
const juce::String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool PluginProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool PluginProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double PluginProcessor::getTailLengthSeconds() const
{
    return 30.0;
}

int PluginProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
        // so this should be at least 1, even if you're not really implementing programs.
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

void PluginProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String PluginProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void PluginProcessor::changeProgramName (int index, const juce::String& newName)
{
    ignoreUnused (index, newName);
}

//==============================================================================
void PluginProcessor::prepareToPlay (const double sampleRate, const int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (samplesPerBlock), static_cast<juce::uint32> (getMainBusNumOutputChannels()) };

    effectsChain.prepare (spec);

    synth.prepareVoices (sampleRate, samplesPerBlock, getTotalNumOutputChannels());

    // Report oversampling latency to the host so it can compensate for audio/MIDI sync
    if (auto* voice = dynamic_cast<MySynthVoice*> (synth.getVoice (0)))
        setLatencySamples (static_cast<int> (voice->getOversamplingLatency()));

    // Calculate maximum allowed processing time (90% of the theoretical maximum)
    maxAllowedProcessingTimeMs = (samplesPerBlock / sampleRate) * 1000.0 * 0.9;
}

void PluginProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
    #if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    #endif

    return true;
#endif
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    // Start timing the processing
    const double startTime = juce::Time::getMillisecondCounterHiRes();

    keyboardState.processNextMidiBuffer (midiMessages, 0, buffer.getNumSamples(), true);

    // Inject UI wheel values as MIDI messages
    {
        float pendingMod = uiModWheelValue.exchange (-1.0f, std::memory_order_relaxed);
        if (pendingMod >= 0.0f)
        {
            int cc1 = juce::jlimit (0, 127, static_cast<int> (pendingMod * 127.0f));
            midiMessages.addEvent (juce::MidiMessage::controllerEvent (1, 1, cc1), 0);
        }
    }
    {
        int pendingBend = uiPitchBendValue.exchange (-1, std::memory_order_relaxed);
        if (pendingBend >= 0)
            midiMessages.addEvent (juce::MidiMessage::pitchWheel (1, pendingBend), 0);
    }

    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    TempoInfo tempoInfo;
    if (auto* playHead = getPlayHead())
        if (auto pos = playHead->getPosition())
        {
            if (auto bpm = pos->getBpm())
            {
                tempoInfo.bpm = *bpm;
                tempoInfo.hostTempoAvailable = true;
            }
            if (auto ppq = pos->getPpqPosition())
                tempoInfo.ppqPosition = *ppq;
            tempoInfo.isPlaying = pos->getIsPlaying();
        }

    synth.updateParameters (tempoInfo);
    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    synth.resetOutputsIfIdle();

    effectsChain.process (buffer, tempoInfo, modDestOutputs);

    // Oscilloscope waveform capture
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<MySynthVoice*> (synth.getVoice (i)))
        {
            if (voice->isVoiceActive())
            {
                std::array<float, 1024> tempVoiceData {};
                const int samplesRead = voice->readWaveformData (tempVoiceData.data(), 1024);
                if (samplesRead > 0)
                    waveformCapture.write (tempVoiceData.data(), samplesRead);
                break;
            }
        }
    }

    // Calculate processing time
    lastProcessingTimeMs = juce::Time::getMillisecondCounterHiRes() - startTime;

    // Detect potential overload (log only — raise(SIGUSR1) crashes outside debugger)
    if (lastProcessingTimeMs > maxAllowedProcessingTimeMs)
    {
        DBG ("AUDIO OVERLOAD DETECTED: Processing took " + juce::String (lastProcessingTimeMs) + "ms, limit is " + juce::String (maxAllowedProcessingTimeMs) + "ms");
    }
}

//==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

//==============================================================================
void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ValueTree state ("PluginState"); // Get the ValueTree state

    state.addChild (parameters.copyState(), -1, nullptr);

    state.addChild (modTree.createCopy(), -1, nullptr);

    for (int i = 0; i < 4; ++i)
    {
        auto lfoTree = lfoData[i]->toValueTree();
        lfoTree.setProperty ("lfoIndex", i, nullptr);
        state.addChild (lfoTree, -1, nullptr);
    }

    // Convert to XML and write to MemoryBlock
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    juce::MemoryOutputStream stream (destData, true);
    xml->writeTo (stream);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Create a MemoryInputStream from the provided data
    juce::MemoryInputStream inputStream (data, static_cast<size_t> (sizeInBytes), false);

    // Read the stream into a String
    juce::String xmlString = inputStream.readString();

    // Parse the string as XML
    std::unique_ptr<juce::XmlElement> xml = juce::XmlDocument::parse (xmlString);
    if (xml != nullptr)
    {
        // Convert XML to ValueTree
        juce::ValueTree state = juce::ValueTree::fromXml (*xml);

        // Restore APVTS state (look for the child with the APVTS identifier, e.g., "MyPlugin")
        juce::ValueTree paramState = state.getChildWithName (parameters.state.getType());
        if (paramState.isValid())
        {
            parameters.replaceState (paramState);
        }

        // Restore custom state — replace children in-place to preserve the
        // SharedObject identity so that MySynthVoice listeners stay valid.
        juce::ValueTree customStateRestored = state.getChildWithName (modTree.getType());
        if (customStateRestored.isValid())
        {
            // Remove existing children
            while (modTree.getNumChildren() > 0)
                modTree.removeChild (0, nullptr);

            // Re-add from restored state
            for (int i = 0; i < customStateRestored.getNumChildren(); ++i)
                modTree.appendChild (customStateRestored.getChild (i).createCopy(), nullptr);

            // Backward compat: pad to 12 children if saved state had fewer
            while (modTree.getNumChildren() < 12)
            {
                juce::ValueTree child ("modulation" + juce::String (modTree.getNumChildren()));
                child.setProperty ("source", "None", nullptr);
                child.setProperty ("depth", 0.0f, nullptr);
                child.setProperty ("destination", "None", nullptr);
                child.setProperty ("isBipolar", false, nullptr);
                child.setProperty ("bypassed", false, nullptr);
                modTree.appendChild (child, nullptr);
            }
        }

        // Restore LFO state
        bool foundIndexed = false;
        for (int i = 0; i < state.getNumChildren(); ++i)
        {
            auto child = state.getChild (i);
            if (child.getType().toString() == "LFOData" && child.hasProperty ("lfoIndex"))
            {
                int idx = static_cast<int> (child.getProperty ("lfoIndex"));
                if (idx >= 0 && idx < 4)
                    lfoData[idx]->fromValueTree (child);
                foundIndexed = true;
            }
        }
        // Backward compat: old single-LFOData presets without lfoIndex
        if (!foundIndexed)
        {
            juce::ValueTree lfoState = state.getChildWithName ("LFOData");
            if (lfoState.isValid())
                lfoData[0]->fromValueTree (lfoState);
        }
    }
}

//==============================================================================
// This creates new instances of the plugin...
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}