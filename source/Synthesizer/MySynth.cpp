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

    monoMode = *parameters.getRawParameterValue ("monoOn") > 0.5f;
    legatoMode = *parameters.getRawParameterValue ("legatoOn") > 0.5f;

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

void MySynth::noteOn (int midiChannel, int midiNoteNumber, float velocity)
{
    const bool sameNote = (midiNoteNumber == lastMidiNote);
    const float fromFreq = lastNoteFrequency;

    if (monoMode && legatoMode)
    {
        // === Legato mode ===
        if (!heldNotes.empty() && !sameNote)
        {
            // Find the voice playing the current note and glide it
            for (int i = 0; i < voices.size(); ++i)
            {
                if (auto* v = dynamic_cast<MySynthVoice*> (voices.getUnchecked (i)))
                {
                    if (v->isVoiceActive() && v->getCurrentlyPlayingNote() == heldNotes.back())
                    {
                        v->glideToNote (midiNoteNumber, velocity);
                        legatoVoice = v;
                        break;
                    }
                }
            }
            lastMidiNote = midiNoteNumber;
            lastNoteFrequency = static_cast<float> (juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber));
            heldNotes.push_back (midiNoteNumber);
            return;
        }

        // First note or same note — normal allocation
        applyToAllVoices ([fromFreq, sameNote] (MySynthVoice* voice)
        {
            voice->setPortamentoStart (fromFreq, sameNote);
        });
        Synthesiser::noteOn (midiChannel, midiNoteNumber, velocity);
    }
    else if (monoMode)
    {
        // === Mono mode ===
        if (!heldNotes.empty())
            Synthesiser::noteOff (midiChannel, heldNotes.back(), 0.0f, true);

        applyToAllVoices ([fromFreq, sameNote] (MySynthVoice* voice)
        {
            voice->setPortamentoStart (fromFreq, sameNote);
        });
        Synthesiser::noteOn (midiChannel, midiNoteNumber, velocity);
    }
    else
    {
        // === Poly mode ===
        applyToAllVoices ([fromFreq, sameNote] (MySynthVoice* voice)
        {
            voice->setPortamentoStart (fromFreq, sameNote);
        });
        Synthesiser::noteOn (midiChannel, midiNoteNumber, velocity);
    }

    lastMidiNote = midiNoteNumber;
    lastNoteFrequency = static_cast<float> (juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber));
    heldNotes.push_back (midiNoteNumber);
}

void MySynth::noteOff (int midiChannel, int midiNoteNumber, float velocity, bool allowTailOff)
{
    // Remove the note from heldNotes
    auto it = std::find (heldNotes.begin(), heldNotes.end(), midiNoteNumber);
    if (it != heldNotes.end())
        heldNotes.erase (it);

    if (monoMode && !heldNotes.empty())
    {
        const int returnNote = heldNotes.back();
        const float returnFreq = static_cast<float> (juce::MidiMessage::getMidiNoteInHertz (returnNote));

        if (legatoMode && legatoVoice != nullptr)
        {
            // Legato: glide back without retrigger
            legatoVoice->glideToNote (returnNote, velocity > 0.0f ? velocity : 0.8f);
        }
        else
        {
            // Mono: release current, retrigger previous
            Synthesiser::noteOff (midiChannel, midiNoteNumber, velocity, allowTailOff);
            applyToAllVoices ([returnFreq] (MySynthVoice* voice)
            {
                voice->setPortamentoStart (returnFreq, false);
            });
            // Use the frequency of the note being released as portamento start
            const float releasedFreq = static_cast<float> (juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber));
            applyToAllVoices ([releasedFreq] (MySynthVoice* voice)
            {
                voice->setPortamentoStart (releasedFreq, false);
            });
            Synthesiser::noteOn (midiChannel, returnNote, velocity > 0.0f ? velocity : 0.8f);
        }

        lastMidiNote = returnNote;
        lastNoteFrequency = returnFreq;
        return;
    }

    // Normal release
    Synthesiser::noteOff (midiChannel, midiNoteNumber, velocity, allowTailOff);

    if (heldNotes.empty())
        legatoVoice = nullptr;
}
