#include "MySynth.h"

void MySynth::setUpSynth()
{
    clearVoices();
    for (int i = 0; i < 16; ++i)
        addVoice (new MySynthVoice());

    clearSounds();
    addSound (new MySynthSound());
}

void MySynth::setVolume (float newVolume)
{
    volume = newVolume;
    for (int i = 0; i < voices.size(); ++i)
    {
        if (auto* voice = dynamic_cast<MySynthVoice*> (voices.getUnchecked (i)))
            voice->setVolume (volume);
    }
}

void MySynth::setFilterCutoff (float newFilterFrequency)
{
    filterCutoff = newFilterFrequency;
    for (int i = 0; i < voices.size(); ++i)
    {
        if (auto* voice = dynamic_cast<MySynthVoice*> (voices.getUnchecked (i)))
            voice->setFilterCutoff (filterCutoff);
    }
}

void MySynth::setFilterEnvelopeAmount (float newFilterEnvelopeAmount)
{
    filterEnvelopeAmount = newFilterEnvelopeAmount;
    for (int i = 0; i < voices.size(); ++i)
    {
        if (auto* voice = dynamic_cast<MySynthVoice*> (voices.getUnchecked (i)))
            voice->setFilterEnvelopeAmount (filterEnvelopeAmount);
    }
}

void MySynth::setFilterResonance (float newFilterResonance)
{
    filterResonance = newFilterResonance;
    for (int i = 0; i < voices.size(); ++i)
    {
        if (auto* voice = dynamic_cast<MySynthVoice*> (voices.getUnchecked (i)))
            voice->setFilterResonance (filterResonance);
    }
}