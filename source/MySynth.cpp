#include "MySynth.h"

MySynth::MySynth (juce::AudioProcessorValueTreeState& p, juce::ValueTree& mt) : parameters (p), modTree (mt)
{
    clearVoices();
    for (int i = 0; i < 1; ++i)
        addVoice (new MySynthVoice (parameters, modTree, ampEnvPtr));

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
        // { "ampAttack", ampAttack, [this] (float value) { applyToAllVoices ([value] (MySynthVoice* voice) { voice->setAmpAttack (value); }); } },
        // { "ampAttackCurve", ampAttackCurve, [this] (float value) { applyToAllVoices ([value] (MySynthVoice* voice) { voice->setAmpAttackCurve (value); }); } },
        // { "ampDecay", ampDecay, [this] (float value) { applyToAllVoices ([value] (MySynthVoice* voice) { voice->setAmpDecay (value); }); } },
        // { "ampDecayCurve", ampDecayCurve, [this] (float value) { applyToAllVoices ([value] (MySynthVoice* voice) { voice->setAmpDecayCurve (value); }); } },
        // { "ampSustain", ampSustain, [this] (float value) { applyToAllVoices ([value] (MySynthVoice* voice) { voice->setAmpSustain (value); }); } },
        // { "ampRelease", ampRelease, [this] (float value) { applyToAllVoices ([value] (MySynthVoice* voice) { voice->setAmpRelease (value); }); } },
        // { "ampReleaseCurve", ampReleaseCurve, [this] (float value) { applyToAllVoices ([value] (MySynthVoice* voice) { voice->setAmpReleaseCurve (value); }); } },
        // { "filterAttack", filterAttack, [this] (float value) { applyToAllVoices ([value] (MySynthVoice* voice) { voice->setFilterAttack (value); }); } },
        // { "filterAttackCurve", filterAttackCurve, [this] (float value) { applyToAllVoices ([value] (MySynthVoice* voice) { voice->setFilterAttackCurve (value); }); } },
        // { "filterDecay", filterDecay, [this] (float value) { applyToAllVoices ([value] (MySynthVoice* voice) { voice->setFilterDecay (value); }); } },
        // { "filterDecayCurve", filterDecayCurve, [this] (float value) { applyToAllVoices ([value] (MySynthVoice* voice) { voice->setFilterDecayCurve (value); }); } },
        // { "filterSustain", filterSustain, [this] (float value) { applyToAllVoices ([value] (MySynthVoice* voice) { voice->setFilterSustain (value); }); } },
        // { "filterRelease", filterRelease, [this] (float value) { applyToAllVoices ([value] (MySynthVoice* voice) { voice->setFilterRelease (value); }); } },
        // { "filterReleaseCurve", filterReleaseCurve, [this] (float value) { applyToAllVoices ([value] (MySynthVoice* voice) { voice->setFilterReleaseCurve (value); }); } },
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
