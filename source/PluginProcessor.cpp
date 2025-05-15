#include "PluginProcessor.h"
#include "MySynthVoice.h"
#include "PluginEditor.h"
#include <csignal>

// Signal handler for audio overloads
void audioOverloadHandler (int signal)
{
    // This will cause your program to break into the debugger
    raise (SIGTRAP);
}

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
      synth (parameters, modTree, pitchTracker, voicePtr),
      waveFifo (1024),
      waveData (1024),
      lastProcessingTimeMs (0.0),
      maxAllowedProcessingTimeMs (0.0)
{
    // Set up signal handler for catching overloads
    signal (SIGUSR1, audioOverloadHandler);

    for (size_t i = 0; i < modList.size(); i++)
    {
        juce::ValueTree childNode (juce::String ("modulation" + i));
        childNode.setProperty ("source", std::get<0> (modList[i]), nullptr);
        childNode.setProperty ("depth", std::get<1> (modList[i]), nullptr);
        childNode.setProperty ("destination", std::get<2> (modList[i]), nullptr);
        childNode.setProperty ("isBipolar", std::get<3> (modList[i]), nullptr);
        modTree.appendChild (childNode, nullptr);
    }
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
    return 0.0;
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
    // juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (samplesPerBlock), static_cast<juce::uint32> (getMainBusNumOutputChannels()) };

    synth.setCurrentPlaybackSampleRate (sampleRate);
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<MySynthVoice*> (synth.getVoice (i)))
        {
            voice->prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
        }
    }

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
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    synth.updateParameters();
    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

    if (*voicePtr != nullptr)
    {
        const auto& voiceBuffer = (*voicePtr)->getWaveformBuffer();
        const int numSamples = voiceBuffer.getNumSamples();
        // const int numSamples = juce::jmin<int> (1024, 2 * static_cast<int> (getSampleRate() / pitchTracker->frequency));
        // const juce::AbstractFifo::ScopedWrite writeHandler = waveFifo.write (numSamples);
        // for (int i = 0; i < writeHandler.blockSize1; ++i)
        for (int i = 0; i < numSamples; ++i)
        {
            // waveData[writeHandler.startIndex1 + i] = voiceBuffer.getSample (0, i);
            waveData[i] = voiceBuffer.getSample (0, i);
        }
        // {
        //     waveData[writeHandler.startIndex1 + i] = voiceBuffer.getSample (0, i);
        // }
    }

    // Calculate processing time
    lastProcessingTimeMs = juce::Time::getMillisecondCounterHiRes() - startTime;

    // Detect potential overload and trigger debugger break
    if (lastProcessingTimeMs > maxAllowedProcessingTimeMs)
    {
        DBG ("AUDIO OVERLOAD DETECTED: Processing took " + juce::String (lastProcessingTimeMs) + "ms, limit is " + juce::String (maxAllowedProcessingTimeMs) + "ms");

        // This will trigger our signal the handler which will break into the debugger
        raise (SIGUSR1);
    }
}

bool PluginProcessor::getWaveformData (float* dest, const int maxSamples)
{
    // const int numReady = waveFifo.getNumReady();
    // if (numReady == 0)
    //     return false;
    // const int numToRead = juce::jmin (numReady, maxSamples);
    // const auto readHandler = waveFifo.read (numToRead);
    for (int i = 0; i < 1024; ++i)
    {
        // dest[i] = waveData[readHandler.startIndex1 + i];
        dest[i] = waveData[i];
    }
    return true;
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

        // Restore custom state (look for the child with the custom identifier, e.g., "CustomState")
        juce::ValueTree customStateRestored = state.getChildWithName (modTree.getType());
        if (customStateRestored.isValid())
        {
            modTree = customStateRestored.createCopy();
        }
    }
}

//==============================================================================
// This creates new instances of the plugin...
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}