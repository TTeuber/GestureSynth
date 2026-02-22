#include "MySynth.h"

MySynth::MySynth (juce::AudioProcessorValueTreeState& p, juce::ValueTree& mt, std::shared_ptr<PitchTracker> pt, std::array<std::shared_ptr<LFOData>, 4>& lfoData) : parameters (p), modTree (mt)
{
    clearVoices();
    for (int i = 0; i < 8; ++i)
        addVoice (new MySynthVoice (parameters, modTree, ampEnvPtr, pt, lfoData));

    clearSounds();
    addSound (new MySynthSound());
}
void MySynth::updateParameter (float& currentValue, const float newValue, const std::function<void (float)>& setterFunction)
{
    if (currentValue != newValue)
    {
        currentValue = newValue;
        setterFunction (currentValue);
    }
}
/**
 * Updates all synth parameters by checking for changes in the AudioProcessorValueTreeState.
 * This function handles the synchronization of various synth parameters including:
 * - Volume and filter settings
 * When a parameter change is detected, it applies the new value to all voices in the synth.
 */
void MySynth::updateParameters (const TempoInfo& tempoInfo)
{
    const float newVolume = *parameters.getRawParameterValue ("volume");
    if (masterVolume != newVolume)
    {
        masterVolume = newVolume;
        applyToAllVoices ([newVolume] (MySynthVoice* voice) { voice->setVolume (newVolume); });
    }

    const float newNoiseLevel = *parameters.getRawParameterValue ("noiseLevel");
    if (noiseLevel != newNoiseLevel)
    {
        noiseLevel = newNoiseLevel;
        applyToAllVoices ([newNoiseLevel] (MySynthVoice* voice) { voice->setNoiseLevel (newNoiseLevel); });
    }

    const float newFilterCutoff = *parameters.getRawParameterValue ("filterFrequency");
    if (filterCutoff != newFilterCutoff)
    {
        filterCutoff = newFilterCutoff;
        applyToAllVoices ([newFilterCutoff] (MySynthVoice* voice) { voice->setFilterCutoff (newFilterCutoff); });
    }

    const float newFilterResonance = *parameters.getRawParameterValue ("filterResonance");
    if (filterResonance != newFilterResonance)
    {
        filterResonance = newFilterResonance;
        applyToAllVoices ([newFilterResonance] (MySynthVoice* voice) { voice->setFilterResonance (newFilterResonance); });
    }

    // LFO rates: tempo sync or free Hz — for all 4 LFOs
    const float newManualBpm = *parameters.getRawParameterValue ("manualBpm");
    manualBpm = newManualBpm;

    for (int li = 0; li < 4; ++li)
    {
        auto s = std::to_string (li + 1);
        const bool newTempoSync = *parameters.getRawParameterValue ("lfo" + s + "TempoSync") > 0.5f;
        const int newNoteDivision = static_cast<int> (*parameters.getRawParameterValue ("lfo" + s + "NoteDivision"));
        const bool newBeatSync = *parameters.getRawParameterValue ("lfo" + s + "BeatSync") > 0.5f;

        lfoTempoSync[li] = newTempoSync;
        lfoNoteDivision[li] = newNoteDivision;
        lfoBeatSync[li] = newBeatSync;

        if (lfoTempoSync[li])
        {
            const double effectiveBpm = tempoInfo.hostTempoAvailable ? tempoInfo.bpm : static_cast<double> (manualBpm);
            const float syncedRate = static_cast<float> (TempoSync::noteDivisionToHz (effectiveBpm, lfoNoteDivision[li]));

            if (lfoRates[li] != syncedRate)
            {
                lfoRates[li] = syncedRate;
                applyToAllVoices ([li, syncedRate] (MySynthVoice* voice) { voice->setLFORate (li, syncedRate); });
            }

            if (lfoBeatSync[li] && tempoInfo.isPlaying)
            {
                const float phase = static_cast<float> (std::fmod (tempoInfo.ppqPosition / TempoSync::beatsPerCycle[lfoNoteDivision[li]], 1.0));
                applyToAllVoices ([li, phase] (MySynthVoice* voice) { voice->setLFOPhase (li, phase); });
            }
        }
        else
        {
            const float newLfoRate = *parameters.getRawParameterValue ("lfo" + s + "Rate");
            if (lfoRates[li] != newLfoRate)
            {
                lfoRates[li] = newLfoRate;
                applyToAllVoices ([li, newLfoRate] (MySynthVoice* voice) { voice->setLFORate (li, newLfoRate); });
            }
        }
    }

    const float newPortamentoTime = *parameters.getRawParameterValue ("portamentoTime");
    if (portamentoTime != newPortamentoTime)
    {
        portamentoTime = newPortamentoTime;
        applyToAllVoices ([newPortamentoTime] (MySynthVoice* voice) { voice->setPortamentoTime (newPortamentoTime); });
    }

    const float newVibratoRate = *parameters.getRawParameterValue ("vibratoRate");
    if (vibratoRate != newVibratoRate)
    {
        vibratoRate = newVibratoRate;
        applyToAllVoices ([newVibratoRate] (MySynthVoice* voice) { voice->setVibratoRate (newVibratoRate); });
    }

    const float newVibratoDepth = *parameters.getRawParameterValue ("vibratoDepth");
    if (vibratoDepth != newVibratoDepth)
    {
        vibratoDepth = newVibratoDepth;
        applyToAllVoices ([newVibratoDepth] (MySynthVoice* voice) { voice->setVibratoDepth (newVibratoDepth); });
    }

    const bool newVibratoOn = *parameters.getRawParameterValue ("vibratoOn") > 0.5f;
    if (vibratoOn != newVibratoOn)
    {
        vibratoOn = newVibratoOn;
        applyToAllVoices ([newVibratoOn] (MySynthVoice* voice) { voice->setVibratoEnabled (newVibratoOn); });
    }
}

template <typename Func>
void MySynth::applyToAllVoices (Func&& function)
{
    for (int i = 0; i < voices.size(); ++i)
    {
        if (auto* voice = dynamic_cast<MySynthVoice*> (voices.getUnchecked (i)))
            function (voice);
    }
}
