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
    using Parameter = juce::AudioParameterFloat;
    using Normalize = juce::NormalisableRange<float>;

    layout.add (make_unique<Parameter> ("volume", "Volume", 0.0f, 1.0f, 0.5f));

    layout.add (make_unique<Parameter> ("filterFrequency", "Filter Frequency", Normalize (20.f, 20000.f, 0.01f, 0.25f), 20000.0f));
    // layout.add (make_unique<Parameter> ("filterEnvelopeAmount", "Filter Envelope Amount", 0.0f, 1.0f, 0.5f));
    layout.add (make_unique<Parameter> ("filterResonance", "Filter Resonance", 0.0f, 1.0f, 0.0f));

    layout.add (make_unique<Parameter> ("pulseWidth", "Pulse Width", 0.1f, 0.9f, 0.5f));

    for (int i = 1; i < 2; i++)
    {
        layout.add (make_unique<Parameter> ("env" + std::to_string (i) + "Attack", "Envelope " + std::to_string (i) + " Attack", Normalize (0.0f, 10.0f, 0.001f, 0.3f), 0.0f));
        layout.add (make_unique<Parameter> ("env" + std::to_string (i) + "AttackCurve", "Envelope " + std::to_string (i) + " Attack Curve", Normalize (0.1f, 10.0f, 0.001f, 0.3f), 1.0f));
        layout.add (make_unique<Parameter> ("env" + std::to_string (i) + "Decay", "Envelope " + std::to_string (i) + " Decay", Normalize (0.0f, 10.0f, 0.001f, 0.3f), 0.5f));
        layout.add (make_unique<Parameter> ("env" + std::to_string (i) + "DecayCurve", "Envelope " + std::to_string (i) + " Decay Curve", Normalize (0.1f, 10.0f, 0.001f, 0.3f), 1.0f));
        layout.add (make_unique<Parameter> ("env" + std::to_string (i) + "Sustain", "Envelope " + std::to_string (i) + " Sustain", 0.0f, 1.0f, 1.0f));
        layout.add (make_unique<Parameter> ("env" + std::to_string (i) + "Release", "Envelope " + std::to_string (i) + " Release", Normalize (0.0f, 10.0f, 0.001f, 0.3f), 0.0f));
        layout.add (make_unique<Parameter> ("env" + std::to_string (i) + "ReleaseCurve", "Envelope " + std::to_string (i) + " Release Curve", Normalize (0.1f, 10.0f, 0.001f, 0.3f), 1.0f));
    }

    layout.add (make_unique<Parameter> ("fineTune", "Fine Tune", -0.5f, 0.5f, 0.0f));

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
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    ignoreUnused (destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin...
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
