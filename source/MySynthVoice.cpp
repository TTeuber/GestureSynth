#include "MySynthVoice.h"

float MySynthVoice::frequencyToPhaseIncrement (const float frequency) const
{
    return 2.0f * juce::MathConstants<float>::pi * frequency / currentSampleRate;
}

MySynthVoice::MySynthVoice()
{
    ampADSR.setSampleRate (currentSampleRate);

    // Set default ADSR parameters
    ampEnvParams.attack = 0.1f;
    ampEnvParams.decay = 0.5f;
    ampEnvParams.sustain = 0.2f;
    ampEnvParams.release = 0.7f;

    ampADSR.setParameters (ampEnvParams);

    filterADSR.setSampleRate (currentSampleRate);

    filterEnvParams.attack = 0.1f;
    filterEnvParams.decay = 0.5f;
    filterEnvParams.sustain = 0.0f;
    filterEnvParams.release = 0.5f;

    filterADSR.setParameters (filterEnvParams);
}

bool MySynthVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<MySynthSound*> (sound) != nullptr;
}

void MySynthVoice::prepare (double sampleRate, int samplesPerBlock, int numChannels)
{
    juce::dsp::ProcessSpec spec {};
    currentSampleRate = sampleRate;
    ampADSR.setSampleRate (sampleRate);
    filterADSR.setSampleRate (sampleRate);
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = numChannels;

    osc.prepare (spec);
    // osc.reset();
}
void MySynthVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int currentPitchWheelPosition)
{
    osc.setFrequency (static_cast<float> (juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber)));
    setVolume (volume * velocity);
    ampADSR.noteOn();
    filterADSR.noteOn();
}

void MySynthVoice::stopNote (float velocity, bool allowTailOff)
{
    if (allowTailOff)
    {
        ampADSR.noteOff();
        filterADSR.noteOff();
    }
    else
    {
        ampADSR.reset();
        filterADSR.reset();
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
    const juce::dsp::ProcessContextReplacing<float> context (block);

    osc.process (context);
    // for (int sample = 0; sample < numSamples; ++sample)
    // {
    //     osc.processNextSample(tempData[sample]);
    // }

    // ampADSR.applyEnvelopeToBuffer (tempBuffer, 0, numSamples);
    // smoothedGain.applyGain (tempData, numSamples);
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float envVal = ampADSR.getNextSample();

        float filterEnvVal = filterADSR.getNextSample();
        osc.setFilterCutoff (filterCutoff * pow (2, filterEnvVal * filterEnvelopeAmount * 10));

        if (currentGain < envVal)
            currentGain = juce::jmin (currentGain + slewRate, envVal);
        else if (currentGain > envVal)
            currentGain = juce::jmax (currentGain - slewRate, envVal);
        tempDataL[sample] *= currentGain;
        tempDataR[sample] *= currentGain;
    }

    outputBuffer.addFrom (0, startSample, tempBuffer, 0, 0, numSamples);
    outputBuffer.addFrom (1, startSample, tempBuffer, 1, 0, numSamples);
}
