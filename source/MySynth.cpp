#include "MySynth.h"

MySynth::MySynth (juce::AudioProcessorValueTreeState& p, juce::ValueTree& mt, std::shared_ptr<PitchTracker> pt, std::shared_ptr<MySynthVoice*> vp) : parameters (p), modTree (mt)
{
    clearVoices();
    for (int i = 0; i < 8; ++i)
        addVoice (new MySynthVoice (parameters, modTree, ampEnvPtr, pt, vp));

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
struct ParameterUpdater
{
    const char* name;
    float& valueRef;
    std::function<void (float)> setter;
};

/**
 * Updates all synth parameters by checking for changes in the AudioProcessorValueTreeState.
 * This function handles the synchronization of various synth parameters including:
 * - Volume and filter settings
 * - Amplitude envelope parameters (attack, decay, sustain, release and their curves)
 * - Filter envelope parameters (attack, decay, sustain, release and their curves)
 * When a parameter change is detected, it applies the new value to all voices in the synth.
 */
void MySynth::updateParameters()
{
    std::vector<ParameterUpdater> parameterUpdaters = {
        { "volume", masterVolume, [this] (float value) {
             applyToAllVoices ([value] (MySynthVoice* voice) { voice->setVolume (value); });
         } },
        { "filterFrequency", filterCutoff, [this] (float value) { applyToAllVoices ([value] (MySynthVoice* voice) { voice->setFilterCutoff (value); }); } },
        { "filterResonance", filterResonance, [this] (float value) { applyToAllVoices ([value] (MySynthVoice* voice) { voice->setFilterResonance (value); }); } },
    };

    for (const auto& updater : parameterUpdaters)
    {
        const float newValue = *parameters.getRawParameterValue (updater.name);
        updateParameter (updater.valueRef, newValue, updater.setter);
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
