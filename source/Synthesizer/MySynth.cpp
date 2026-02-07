#include "MySynth.h"

MySynth::MySynth (juce::AudioProcessorValueTreeState& p, juce::ValueTree& mt, std::shared_ptr<PitchTracker> pt, std::shared_ptr<LFOData> lfoData) : parameters (p), modTree (mt)
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
void MySynth::updateParameters()
{
    const float newVolume = *parameters.getRawParameterValue ("volume");
    if (masterVolume != newVolume)
    {
        masterVolume = newVolume;
        applyToAllVoices ([newVolume] (MySynthVoice* voice) { voice->setVolume (newVolume); });
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

    const float newLfo1Rate = *parameters.getRawParameterValue ("lfo1Rate");
    if (lfo1Rate != newLfo1Rate)
    {
        lfo1Rate = newLfo1Rate;
        applyToAllVoices ([newLfo1Rate] (MySynthVoice* voice) { voice->setLFORate (newLfo1Rate); });
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
