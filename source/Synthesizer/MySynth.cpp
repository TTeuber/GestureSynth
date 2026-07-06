#include "MySynth.h"

MySynth::MySynth (juce::AudioProcessorValueTreeState& p, juce::ValueTree& mt, std::shared_ptr<PitchTracker> pt, std::array<std::shared_ptr<LFOData>, 4>& lfoData) : parameters (p), modTree (mt), pitchTracker (pt), lfoDataPtr (&lfoData)
{
    // Cache APVTS atomic pointers (stable for lifetime of APVTS)
    volumeParam          = parameters.getRawParameterValue (ParamIDs::volume);
    noiseOnParam         = parameters.getRawParameterValue (ParamIDs::noiseOn);
    filterCutoffParam    = parameters.getRawParameterValue (ParamIDs::filterFrequency);
    filterResonanceParam = parameters.getRawParameterValue (ParamIDs::filterResonance);
    portamentoTimeParam  = parameters.getRawParameterValue (ParamIDs::portamentoTime);
    vibratoOnParam       = parameters.getRawParameterValue (ParamIDs::vibratoOn);
    monoOnParam          = parameters.getRawParameterValue (ParamIDs::monoOn);
    legatoOnParam        = parameters.getRawParameterValue (ParamIDs::legatoOn);
    voiceCountParam      = parameters.getRawParameterValue (ParamIDs::voiceCount);
    manualBpmParam       = parameters.getRawParameterValue (ParamIDs::manualBpm);

    for (int i = 0; i < 4; ++i)
    {
        lfoParams[static_cast<size_t> (i)].rate         = parameters.getRawParameterValue (ParamIDs::lfoParamID (i + 1, "Rate"));
        lfoParams[static_cast<size_t> (i)].tempoSync    = parameters.getRawParameterValue (ParamIDs::lfoParamID (i + 1, "TempoSync"));
        lfoParams[static_cast<size_t> (i)].noteDivision = parameters.getRawParameterValue (ParamIDs::lfoParamID (i + 1, "NoteDivision"));
        lfoParams[static_cast<size_t> (i)].beatSync     = parameters.getRawParameterValue (ParamIDs::lfoParamID (i + 1, "BeatSync"));
        lfoParams[static_cast<size_t> (i)].mono         = parameters.getRawParameterValue (ParamIDs::lfoParamID (i + 1, "Mono"));
    }

    // Note-tracking vectors never exceed the MIDI note range, so reserving up
    // front keeps push_back from allocating on the audio thread.
    heldNotes.reserve (128);
    sustainedNotes.reserve (128);

    clearVoices();
    for (int i = 0; i < 8; ++i)
        addVoice (new MySynthVoice (parameters, modTree, envPtrs, lfoPtrs, pt, lfoData, &currentVelocityRaw, &currentKeyboardRaw, &currentModWheelRaw, &currentPitchBendRaw, &currentAftertouchRaw, &currentExpressionRaw));

    monoVoice = dynamic_cast<MySynthVoice*> (voices.getUnchecked (0));

    prevNoiseOn = noiseOnParam->load() > 0.5f;
    applyToAllVoices ([noise = prevNoiseOn] (MySynthVoice* voice) { voice->setNoiseEnabled (noise); });

    prevVibratoOn = vibratoOnParam->load() > 0.5f;
    applyToAllVoices ([vib = prevVibratoOn] (MySynthVoice* voice) { voice->setVibratoEnabled (vib); });

    clearSounds();
    addSound (new MySynthSound());
}

void MySynth::prepareVoices (double sampleRate, int samplesPerBlock, int numChannels)
{
    preparedSampleRate = sampleRate;
    preparedSamplesPerBlock = samplesPerBlock;
    preparedNumChannels = numChannels;

    setCurrentPlaybackSampleRate (sampleRate);

    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<MySynthVoice*> (getVoice (i)))
            voice->prepare (sampleRate, samplesPerBlock, numChannels);
    }
}

void MySynth::setVoiceCount (int count)
{
    if (count == currentVoiceCount)
        return;

    // Stop all voices and clear held notes
    allNotesOff (0, false);
    heldNotes.clear();
    lastMidiNote = -1;

    clearVoices();
    for (int i = 0; i < count; ++i)
        addVoice (new MySynthVoice (parameters, modTree, envPtrs, lfoPtrs, pitchTracker, *lfoDataPtr,
            &currentVelocityRaw, &currentKeyboardRaw, &currentModWheelRaw,
            &currentPitchBendRaw, &currentAftertouchRaw, &currentExpressionRaw));

    monoVoice = dynamic_cast<MySynthVoice*> (voices.getUnchecked (0));
    currentVoiceCount = count;

    // Push cached MIDI controller state into the freshly-allocated voices so they
    // pick up the current mod wheel / expression / aftertouch instead of starting at 0.
    {
        const int cc1 = juce::jlimit (0, 127, static_cast<int> (currentModWheelRaw.load() * 127.0f));
        const int cc11 = juce::jlimit (0, 127, static_cast<int> (currentExpressionRaw.load() * 127.0f));
        const int at = juce::jlimit (0, 127, static_cast<int> (currentAftertouchRaw.load() * 127.0f));
        applyToAllVoices ([cc1, cc11, at] (MySynthVoice* voice)
        {
            voice->controllerMoved (1, cc1);
            voice->controllerMoved (11, cc11);
            voice->aftertouchChanged (at);
        });
    }

    // Prepare new voices with stored audio settings
    if (preparedSampleRate > 0.0)
        prepareVoices (preparedSampleRate, preparedSamplesPerBlock, preparedNumChannels);

    // Reconnect mod output arrays to new voices
    if (cachedSourceOutputArray != nullptr)
        setSourceOutputArray (cachedSourceOutputArray);
    if (cachedDestOutputArray != nullptr)
        setDestOutputArray (cachedDestOutputArray);

    // Reset previous-value cache so updateParameters() re-applies everything
    prevVolume = -1.0f;
    prevNoiseOn = false;
    prevFilterCutoff = -1.0f;
    prevFilterResonance = -1.0f;
    prevPortamentoTime = -1.0f;
    prevVibratoOn = false;
}

void MySynth::updateParameters (const TempoInfo& tempoInfo)
{
    const float newVolume = volumeParam->load();
    if (! juce::exactlyEqual (prevVolume, newVolume))
    {
        prevVolume = newVolume;
        applyToAllVoices ([newVolume] (MySynthVoice* voice) { voice->setVolume (newVolume); });
    }

    const bool newNoiseOn = noiseOnParam->load() > 0.5f;
    if (prevNoiseOn != newNoiseOn)
    {
        prevNoiseOn = newNoiseOn;
        applyToAllVoices ([newNoiseOn] (MySynthVoice* voice) { voice->setNoiseEnabled (newNoiseOn); });
    }

    const float newFilterCutoff = filterCutoffParam->load();
    if (! juce::exactlyEqual (prevFilterCutoff, newFilterCutoff))
    {
        prevFilterCutoff = newFilterCutoff;
        applyToAllVoices ([newFilterCutoff] (MySynthVoice* voice) { voice->setFilterCutoff (newFilterCutoff); });
    }

    const float newFilterResonance = filterResonanceParam->load();
    if (! juce::exactlyEqual (prevFilterResonance, newFilterResonance))
    {
        prevFilterResonance = newFilterResonance;
        applyToAllVoices ([newFilterResonance] (MySynthVoice* voice) { voice->setFilterResonance (newFilterResonance); });
    }

    // LFO rates: tempo sync or free Hz — for all 4 LFOs
    const float newManualBpm = manualBpmParam->load();

    for (int li = 0; li < 4; ++li)
    {
        const bool newTempoSync = lfoParams[static_cast<size_t> (li)].tempoSync->load() > 0.5f;
        const int newNoteDivision = static_cast<int> (lfoParams[static_cast<size_t> (li)].noteDivision->load());
        const bool newBeatSync = lfoParams[static_cast<size_t> (li)].beatSync->load() > 0.5f;

        if (newTempoSync)
        {
            const double effectiveBpm = tempoInfo.hostTempoAvailable ? tempoInfo.bpm : static_cast<double> (newManualBpm);
            const float syncedRate = static_cast<float> (TempoSync::noteDivisionToHz (effectiveBpm, newNoteDivision));

            if (! juce::exactlyEqual (prevLfoRates[static_cast<size_t> (li)], syncedRate))
            {
                prevLfoRates[static_cast<size_t> (li)] = syncedRate;
                applyToAllVoices ([li, syncedRate] (MySynthVoice* voice) { voice->setLFORate (li, syncedRate); });
            }

            if (newBeatSync && tempoInfo.isPlaying)
            {
                const float phase = static_cast<float> (std::fmod (tempoInfo.ppqPosition / TempoSync::beatsPerCycle[newNoteDivision], 1.0));
                applyToAllVoices ([li, phase] (MySynthVoice* voice) { voice->setLFOPhase (li, phase); });
            }
        }
        else
        {
            const float newLfoRate = lfoParams[static_cast<size_t> (li)].rate->load();
            if (! juce::exactlyEqual (prevLfoRates[static_cast<size_t> (li)], newLfoRate))
            {
                prevLfoRates[static_cast<size_t> (li)] = newLfoRate;
                applyToAllVoices ([li, newLfoRate] (MySynthVoice* voice) { voice->setLFORate (li, newLfoRate); });
            }
        }

        // Mono LFO: copy oldest active voice's phase to all others
        const bool isMono = lfoParams[static_cast<size_t> (li)].mono->load() > 0.5f;
        if (isMono)
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

    const float newPortamentoTime = portamentoTimeParam->load();
    if (! juce::exactlyEqual (prevPortamentoTime, newPortamentoTime))
    {
        prevPortamentoTime = newPortamentoTime;
        applyToAllVoices ([newPortamentoTime] (MySynthVoice* voice) { voice->setPortamentoTime (newPortamentoTime); });
    }

    monoMode = monoOnParam->load() > 0.5f;
    legatoMode = legatoOnParam->load() > 0.5f;

    // Voice count
    static constexpr int voiceCountChoices[] = { 2, 3, 4, 5, 6, 8, 12, 16 };
    const int voiceIdx = static_cast<int> (voiceCountParam->load());
    const int newVoiceCount = voiceCountChoices[juce::jlimit (0, 7, voiceIdx)];
    if (newVoiceCount != currentVoiceCount)
        setVoiceCount (newVoiceCount);

    const bool newVibratoOn = vibratoOnParam->load() > 0.5f;
    if (prevVibratoOn != newVibratoOn)
    {
        prevVibratoOn = newVibratoOn;
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
    cachedSourceOutputArray = arr;
    applyToAllVoices ([arr] (MySynthVoice* v) { v->setSourceOutputArray (arr); });
}

void MySynth::setDestOutputArray (std::atomic<float>* arr)
{
    cachedDestOutputArray = arr;
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
    if (monoMode && sustainPedalDown)
    {
        if (std::find (sustainedNotes.begin(), sustainedNotes.end(), midiNoteNumber) == sustainedNotes.end())
            sustainedNotes.push_back (midiNoteNumber);
        return;
    }

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

void MySynth::handleController (int midiChannel, int controllerNumber, int controllerValue)
{
    // Base class handles sustain/soft-pedal routing and dispatches to voices that
    // are currently playing the channel. We additionally cache CC1/CC11 and push
    // them to *all* voices (including idle ones) so the mod matrix sees the latest
    // value even when no notes are active and so newly-started notes pick it up.
    Synthesiser::handleController (midiChannel, controllerNumber, controllerValue);

    if (controllerNumber == 1 || controllerNumber == 11)
    {
        const float normalized = static_cast<float> (controllerValue) / 127.0f;
        AtomicHelpers::paramStore (controllerNumber == 1 ? currentModWheelRaw : currentExpressionRaw, normalized);
        applyToAllVoices ([controllerNumber, controllerValue] (MySynthVoice* voice)
        {
            voice->controllerMoved (controllerNumber, controllerValue);
        });
    }
}

void MySynth::handleAftertouch (int midiChannel, int midiNoteNumber, int aftertouchValue)
{
    Synthesiser::handleAftertouch (midiChannel, midiNoteNumber, aftertouchValue);

    AtomicHelpers::paramStore (currentAftertouchRaw, static_cast<float> (aftertouchValue) / 127.0f);
    applyToAllVoices ([aftertouchValue] (MySynthVoice* voice)
    {
        voice->aftertouchChanged (aftertouchValue);
    });
}

void MySynth::handleSustainPedal (int midiChannel, bool isDown)
{
    // Let the base class do per-voice sustain bookkeeping (poly mode handled here).
    Synthesiser::handleSustainPedal (midiChannel, isDown);

    sustainPedalDown = isDown;

    if (! isDown && monoMode)
    {
        // Pedal released: process any mono note-offs that were deferred.
        auto deferred = std::move (sustainedNotes);
        sustainedNotes.clear();
        for (int note : deferred)
            noteOff (midiChannel, note, 0.0f, true);
    }
    else if (! isDown)
    {
        sustainedNotes.clear();
    }
}

void MySynth::allNotesOff (int midiChannel, bool allowTailOff)
{
    Synthesiser::allNotesOff (midiChannel, allowTailOff);

    heldNotes.clear();
    sustainedNotes.clear();
    sustainPedalDown = false;
    lastMidiNote = -1;
    lastNoteFrequency = 0.0f;
}
