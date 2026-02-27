#include "MySynth.h"

MySynth::MySynth (juce::AudioProcessorValueTreeState& p, juce::ValueTree& mt, std::shared_ptr<PitchTracker> pt, std::array<std::shared_ptr<LFOData>, 4>& lfoData) : parameters (p), modTree (mt)
{
    clearVoices();
    for (int i = 0; i < 8; ++i)
        addVoice (new MySynthVoice (parameters, modTree, ampEnvPtr, pt, lfoData, &currentVelocityRaw, &currentKeyboardRaw, &currentModWheelRaw, &currentPitchBendRaw));

    monoVoice = dynamic_cast<MySynthVoice*> (voices.getUnchecked (0));

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

        // Mono LFO: copy oldest active voice's phase to all others
        lfoMono[li] = *parameters.getRawParameterValue ("lfo" + s + "Mono") > 0.5f;
        if (lfoMono[li])
        {
            MySynthVoice* oldest = nullptr;
            uint64_t oldestOrder = UINT64_MAX;
            for (int vi = 0; vi < voices.size(); ++vi)
            {
                auto* v = dynamic_cast<MySynthVoice*> (voices.getUnchecked (vi));
                if (v != nullptr && v->isVoiceActive() && v->getVoiceStartOrder() < oldestOrder)
                {
                    oldestOrder = v->getVoiceStartOrder();
                    oldest = v;
                }
            }
            if (oldest != nullptr)
            {
                const float monoPhase = oldest->getLFOPhase (li);
                applyToAllVoices ([li, monoPhase, oldest] (MySynthVoice* voice)
                {
                    if (voice != oldest && voice->isVoiceActive())
                        voice->setLFOPhase (li, monoPhase);
                });
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

void MySynth::setSourceOutputArray (std::atomic<float>* arr)
{
    applyToAllVoices ([arr] (MySynthVoice* v) { v->setSourceOutputArray (arr); });
}

void MySynth::setDestOutputArray (std::atomic<float>* arr)
{
    applyToAllVoices ([arr] (MySynthVoice* v) { v->setDestOutputArray (arr); });
}

void MySynth::resetOutputsIfIdle()
{
    for (int i = 0; i < voices.size(); ++i)
    {
        if (voices.getUnchecked (i)->isVoiceActive())
            return;
    }
    if (auto* voice = dynamic_cast<MySynthVoice*> (voices.getUnchecked (0)))
        voice->resetModOutputs();
}

void MySynth::noteOn (int midiChannel, int midiNoteNumber, float velocity)
{
    const juce::ScopedLock sl (lock);
    const bool sameNote = (midiNoteNumber == lastMidiNote);
    const float fromFreq = lastNoteFrequency;

    if (monoMode)
    {
        // Kill ALL other voices immediately (cleanup from poly mode, prevents stacking)
        for (int i = 0; i < voices.size(); ++i)
        {
            auto* v = voices.getUnchecked (i);
            if (v != monoVoice && v->isVoiceActive())
                stopVoice (v, 0.0f, false);
        }

        if (legatoMode && !heldNotes.empty() && !sameNote)
        {
            // Legato: glide without envelope retrigger
            monoVoice->glideToNote (midiNoteNumber, velocity);
        }
        else
        {
            // Mono (or first note in legato, or same-note retrigger):
            monoVoice->setPortamentoStart (fromFreq, sameNote);
            startVoice (monoVoice, sounds[0], midiChannel, midiNoteNumber, velocity);
        }
    }
    else
    {
        // Poly mode — unchanged
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
    const juce::ScopedLock sl (lock);

    auto it = std::find (heldNotes.begin(), heldNotes.end(), midiNoteNumber);
    if (it != heldNotes.end())
        heldNotes.erase (it);

    if (monoMode && !heldNotes.empty())
    {
        const int returnNote = heldNotes.back();
        const float releasedFreq = static_cast<float> (juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber));

        if (legatoMode)
        {
            // Legato: glide back to previous note, no retrigger
            monoVoice->glideToNote (returnNote, velocity > 0.0f ? velocity : 0.8f);
        }
        else
        {
            // Mono: retrigger with portamento from released note
            monoVoice->setPortamentoStart (releasedFreq, false);
            startVoice (monoVoice, sounds[0], midiChannel, returnNote, velocity > 0.0f ? velocity : 0.8f);
        }

        lastMidiNote = returnNote;
        lastNoteFrequency = static_cast<float> (juce::MidiMessage::getMidiNoteInHertz (returnNote));
        return;
    }

    if (monoMode)
    {
        // All notes released in mono — release the mono voice with tail-off
        stopVoice (monoVoice, velocity, allowTailOff);
    }
    else
    {
        // Poly mode — unchanged
        Synthesiser::noteOff (midiChannel, midiNoteNumber, velocity, allowTailOff);
    }
}
