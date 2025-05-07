#include "PluginProcessor.h"

#include "MySynthVoice.h"
#include "PluginEditor.h"

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
      waveData (1024)
{
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

juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createLayout() const
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    using namespace std;
    using namespace juce;
    using Parameter = AudioParameterFloat;
    using Normalize = NormalisableRange<float>;

    layout.add (make_unique<Parameter> (ParameterID ("volume", 1), "Volume", 0.0f, 1.0f, 0.5f));

    auto filterFrequency = make_unique<Parameter> (ParameterID ("filterFrequency", 1), "Filter Frequency", Normalize (20.f, 20000.f, 0.01f, 0.25f), 20000.0f);
    filterFrequency->range.setSkewForCentre (1200.0f);
    layout.add (std::move (filterFrequency));

    auto filterResonance = make_unique<Parameter> (ParameterID ("filterResonance", 1), "Filter Resonance", Normalize (0.1, 10, 0.01, 0.45), 0.71f);
    filterResonance->range.setSkewForCentre (1.0f);
    layout.add (std::move (filterResonance));

    layout.add (make_unique<Parameter> (ParameterID ("oscWaveform", 1), "Oscillator Waveform", 0.0f, 1.0f, 0.5f));
    layout.add (make_unique<Parameter> (ParameterID ("oscDetune", 1), "Oscillator Detune", 0.0f, 1.0f, 0.0f));
    layout.add (make_unique<Parameter> (ParameterID ("oscWidth", 1), "Oscillator Width", 0.5f, 1.0f, 1.0f));
    layout.add (make_unique<Parameter> (ParameterID ("subOsc", 1), "Sub Oscillator", 0.0f, 1.0f, 0.0f));

    layout.add (make_unique<Parameter> (ParameterID ("chorusMix", 1), "Chorus Mix", 0.0f, 1.0f, 0.5f));

    layout.add (make_unique<Parameter> (ParameterID ("pulseWidth", 1), "Pulse Width", 0.1f, 0.9f, 0.5f));

    for (int i = 1; i < 2; i++)
    {
        layout.add (make_unique<Parameter> (ParameterID ("env" + std::to_string (i) + "Attack", 1), "Envelope " + std::to_string (i) + " Attack", Normalize (0.01f, 10.0f, 0.001f, 0.3f), 0.0f));
        layout.add (make_unique<Parameter> (ParameterID ("env" + std::to_string (i) + "AttackCurve", 1), "Envelope " + std::to_string (i) + " Attack Curve", Normalize (0.1f, 10.0f, 0.001f, 0.3f), 1.0f));
        layout.add (make_unique<Parameter> (ParameterID ("env" + std::to_string (i) + "Decay", 1), "Envelope " + std::to_string (i) + " Decay", Normalize (0.01f, 10.0f, 0.001f, 0.3f), 0.5f));
        layout.add (make_unique<Parameter> (ParameterID ("env" + std::to_string (i) + "DecayCurve", 1), "Envelope " + std::to_string (i) + " Decay Curve", Normalize (0.1f, 10.0f, 0.001f, 0.3f), 1.0f));
        layout.add (make_unique<Parameter> (ParameterID ("env" + std::to_string (i) + "Sustain", 1), "Envelope " + std::to_string (i) + " Sustain", 0.0f, 1.0f, 1.0f));
        layout.add (make_unique<Parameter> (ParameterID ("env" + std::to_string (i) + "Release", 1), "Envelope " + std::to_string (i) + " Release", Normalize (0.01f, 10.0f, 0.001f, 0.3f), 0.0f));
        layout.add (make_unique<Parameter> (ParameterID ("env" + std::to_string (i) + "ReleaseCurve", 1), "Envelope " + std::to_string (i) + " Release Curve", Normalize (0.1f, 10.0f, 0.001f, 0.3f), 1.0f));
    }

    layout.add (make_unique<Parameter> (ParameterID ("fineTune", 1), "Fine Tune", -0.5f, 0.5f, 0.0f));

    return layout;
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
    keyboardState.processNextMidiBuffer (midiMessages, 0, buffer.getNumSamples(), true);
    ignoreUnused (midiMessages);

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
