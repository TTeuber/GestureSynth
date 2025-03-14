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
      synth (parameters)
{
}

juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    using namespace std;
    using Parameter = juce::AudioParameterFloat;
    using Normalize = juce::NormalisableRange<float>;

    layout.add (make_unique<Parameter> ("volume", "Volume", 0.0f, 1.0f, 0.5f));

    layout.add (make_unique<Parameter> ("filterFrequency", "Filter Frequency", Normalize (20.f, 20000.f, 0.01f, 0.25f), 1000.0f));
    layout.add (make_unique<Parameter> ("filterEnvelopeAmount", "Filter Envelope Amount", 0.0f, 1.0f, 0.5f));
    layout.add (make_unique<Parameter> ("filterResonance", "Filter Resonance", 0.0f, 1.0f, 0.5f));

    layout.add (make_unique<Parameter> ("ampAttack", "Amp Attack", Normalize (0.0f, 10.0f, 0.001f, 0.3f), 0.0f));
    layout.add (make_unique<Parameter> ("ampDecay", "Amp Decay", Normalize (0.0f, 10.0f, 0.001f, 0.3f), 0.5f));
    layout.add (make_unique<Parameter> ("ampSustain", "Amp Sustain", 0.0f, 1.0f, 0.5f));
    layout.add (make_unique<Parameter> ("ampRelease", "Amp Release", Normalize (0.0f, 10.0f, 0.001f, 0.3f), 0.5f));

    layout.add (make_unique<Parameter> ("filterAttack", "Filter Attack", Normalize (0.0f, 10.0f, 0.001f, 0.3f), 0.0f));
    layout.add (make_unique<Parameter> ("filterDecay", "Filter Decay", Normalize (0.0f, 10.0f, 0.001f, 0.3f), 0.5f));
    layout.add (make_unique<Parameter> ("filterSustain", "Filter Sustain", 0.0f, 1.0f, 0.5f));
    layout.add (make_unique<Parameter> ("filterRelease", "Filter Release", Normalize (0.0f, 10.0f, 0.001f, 0.3f), 0.5f));

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
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (samplesPerBlock), static_cast<juce::uint32> (getMainBusNumOutputChannels()) };

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
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    synth.updateParameters();
    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    tick();
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
    juce::ignoreUnused (destData);
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
