#pragma once

#include "MySynthVoice.h"
#include <juce_dsp/juce_dsp.h>

class MySynth : public juce::Synthesiser
{
public:
    void setUpSynth();

    float getVolume() const { return volume; }
    void setVolume (float newVolume);

    float getFilterCutoff() const { return filterCutoff; }
    void setFilterCutoff (float newFilterFrequency);

    float getFilterEnvelopeAmount() const { return filterEnvelopeAmount; }
    void setFilterEnvelopeAmount (float newFilterEnvelopeAmount);

    float getFilterResonance() const { return filterResonance; }
    void setFilterResonance (float newFilterResonance);

private:
    float volume = 0.0f;
    float filterCutoff = 0.0f;
    float filterEnvelopeAmount = 0.0f;
    float filterResonance = 0.0f;
};
