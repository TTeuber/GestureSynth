//
// Created by Tyler Teuber on 5/10/25.
//

#pragma once

#include "juce_audio_processors/juce_audio_processors.h"

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    using namespace std;
    using namespace juce;
    using Parameter = AudioParameterFloat;
    using Normalize = NormalisableRange<float>;

    // ================================================================================================================================================
    // Global Parameters
    layout.add (make_unique<Parameter> (ParameterID ("volume", 1), "Volume", 0.0f, 1.0f, 0.5f));
    layout.add (make_unique<Parameter> (ParameterID ("noiseLevel", 1), "Noise", 0.0f, 1.0f, 0.0f));

    // ================================================================================================================================================
    // Filter Parameters
    auto filterFrequency = make_unique<Parameter> (ParameterID ("filterFrequency", 1), "Filter Frequency", Normalize (20.f, 20000.f, 0.01f, 0.25f), 20000.0f);
    filterFrequency->range.setSkewForCentre (1200.0f);
    layout.add (std::move (filterFrequency));

    auto filterResonance = make_unique<Parameter> (ParameterID ("filterResonance", 1), "Filter Resonance", Normalize (0.1, 10, 0.01, 0.45), 0.71f);
    filterResonance->range.setSkewForCentre (1.0f);
    layout.add (std::move (filterResonance));

    auto hpfFrequency = make_unique<Parameter> (ParameterID ("hpfFrequency", 1), "HPF Frequency", Normalize (20.f, 3000.f, 0.01f, 0.25f), 20.0f);
    hpfFrequency->range.setSkewForCentre (300.0f);
    layout.add (std::move (hpfFrequency));

    // ================================================================================================================================================
    // Oscillator Parameters
    layout.add (make_unique<Parameter> (ParameterID ("oscWaveform", 1), "Waveform", 0.0f, 1.0f, 0.5f));
    layout.add (make_unique<Parameter> (ParameterID ("pulseWidth", 1), "Pulse Width", 0.1f, 0.9f, 0.5f));
    layout.add (make_unique<Parameter> (ParameterID ("oscDetune", 1), "Detune", 0.0f, 1.0f, 0.2f));
    layout.add (make_unique<Parameter> (ParameterID ("oscWidth", 1), "Width", 0.0f, 1.0f, 1.0f));
    layout.add (make_unique<Parameter> (ParameterID ("subOsc", 1), "Sub Level", 0.0f, 1.0f, 0.75f));
    layout.add (make_unique<Parameter> (ParameterID ("subOscWave", 2), "Sub Wave", 0.0f, 1.0f, 0.67f));
    layout.add (make_unique<Parameter> (ParameterID ("fineTune", 1), "Fine Tune", -0.5f, 0.5f, 0.0f));

    // ================================================================================================================================================
    // Chorus Parameters
    layout.add (make_unique<Parameter> (ParameterID ("chorusMix", 1), "Chorus Mix", 0.0f, 1.0f, 0.5f));
    layout.add (make_unique<Parameter> (ParameterID ("chorusDepth", 1), "Chorus Depth", Normalize (0.0f, 0.03f, 0.0f), 0.015f));
    auto chorusRate = make_unique<Parameter> (ParameterID ("chorusRate", 1), "Chorus Rate", Normalize (0.0f, 1.0f, 0.0f), 0.15f);
    chorusRate->range.setSkewForCentre (0.3f);
    layout.add (std::move (chorusRate));

    // ================================================================================================================================================
    // Vibrato Parameters
    auto vibratoRate = make_unique<Parameter> (ParameterID ("vibratoRate", 1), "Vibrato Rate",
        Normalize (0.1f, 20.0f, 0.01f, 0.35f), 5.0f);
    vibratoRate->range.setSkewForCentre (5.0f);
    layout.add (std::move (vibratoRate));
    layout.add (make_unique<Parameter> (ParameterID ("vibratoDepth", 1), "Vibrato Depth",
        Normalize (0.0f, 2.0f, 0.001f), 0.0f));

    // ================================================================================================================================================
    // LFO Parameters
    for (int i = 1; i <= 4; i++)
    {
        auto s = std::to_string (i);
        auto lfoRate = make_unique<Parameter> (ParameterID ("lfo" + s + "Rate", 1), "LFO " + s + " Rate", Normalize (0.01f, 20.0f, 0.001f, 0.35f), 1.0f);
        lfoRate->range.setSkewForCentre (2.0f);
        layout.add (std::move (lfoRate));

        layout.add (make_unique<AudioParameterBool> (ParameterID ("lfo" + s + "TempoSync", 1), "LFO " + s + " Tempo Sync", false));
        layout.add (make_unique<AudioParameterChoice> (ParameterID ("lfo" + s + "NoteDivision", 1), "LFO " + s + " Note Division",
            juce::StringArray { "4/1", "2/1", "1/1", "1/2", "1/2T", "1/2D", "1/4", "1/4T", "1/4D", "1/8", "1/8T", "1/8D", "1/16", "1/16T", "1/16D", "1/32" }, 6));
        layout.add (make_unique<AudioParameterBool> (ParameterID ("lfo" + s + "BeatSync", 1), "LFO " + s + " Beat Sync", false));
    }

    auto manualBpm = make_unique<Parameter> (ParameterID ("manualBpm", 1), "Manual BPM", Normalize (20.0f, 300.0f, 0.1f), 120.0f);
    layout.add (std::move (manualBpm));

    // ================================================================================================================================================
    // Boolean Parameters
    layout.add (make_unique<AudioParameterBool> (ParameterID ("oscOn", 1), "Osc On", true));
    layout.add (make_unique<AudioParameterBool> (ParameterID ("detuneOn", 1), "Detune On", false));
    layout.add (make_unique<AudioParameterBool> (ParameterID ("subOn", 1), "Sub On", false));
    layout.add (make_unique<AudioParameterBool> (ParameterID ("filterOn", 1), "Filter On", true));
    layout.add (make_unique<AudioParameterBool> (ParameterID ("hpfOn", 1), "HPF On", true));
    layout.add (make_unique<AudioParameterBool> (ParameterID ("chorusOn", 1), "Chorus On", false));
    layout.add (make_unique<AudioParameterBool> (ParameterID ("vibratoOn", 1), "Vibrato On", true));

    // ================================================================================================================================================
    // Portamento Parameters
    auto portamentoTime = make_unique<Parameter> (ParameterID ("portamentoTime", 1), "Portamento", Normalize (0.0f, 5000.0f, 0.1f), 0.0f);
    portamentoTime->range.setSkewForCentre (500.0f);
    layout.add (std::move (portamentoTime));

    // ================================================================================================================================================
    // Voice Mode Parameters
    layout.add (make_unique<AudioParameterBool> (ParameterID ("monoOn", 1), "Mono Mode", false));
    layout.add (make_unique<AudioParameterBool> (ParameterID ("legatoOn", 1), "Legato", false));
    layout.add (make_unique<AudioParameterBool> (ParameterID ("gateMode", 1), "Gate Mode", false));
    layout.add (make_unique<Parameter> (ParameterID ("pitchBendRange", 1), "Pitch Bend Range", Normalize (1.0f, 12.0f, 1.0f), 2.0f));

    // ================================================================================================================================================
    // Envelope Parameters

    for (int i = 1; i <= 4; i++)
    {
        layout.add (make_unique<Parameter> (ParameterID ("env" + std::to_string (i) + "Attack", 1), "Envelope " + std::to_string (i) + " Attack", Normalize (0.01f, 10.0f, 0.001f, 0.3f), 0.0f));
        layout.add (make_unique<Parameter> (ParameterID ("env" + std::to_string (i) + "AttackCurve", 1), "Envelope " + std::to_string (i) + " Attack Curve", Normalize (-1.0f, 1.0f, 0.01f), 0.0f));
        layout.add (make_unique<Parameter> (ParameterID ("env" + std::to_string (i) + "Decay", 1), "Envelope " + std::to_string (i) + " Decay", Normalize (0.01f, 10.0f, 0.001f, 0.3f), 0.5f));
        layout.add (make_unique<Parameter> (ParameterID ("env" + std::to_string (i) + "DecayCurve", 1), "Envelope " + std::to_string (i) + " Decay Curve", Normalize (-1.0f, 1.0f, 0.01f), 0.25f));
        layout.add (make_unique<Parameter> (ParameterID ("env" + std::to_string (i) + "Sustain", 1), "Envelope " + std::to_string (i) + " Sustain", 0.0f, 1.0f, 1.0f));
        layout.add (make_unique<Parameter> (ParameterID ("env" + std::to_string (i) + "Release", 1), "Envelope " + std::to_string (i) + " Release", Normalize (0.01f, 10.0f, 0.001f, 0.3f), 0.0f));
        layout.add (make_unique<Parameter> (ParameterID ("env" + std::to_string (i) + "ReleaseCurve", 1), "Envelope " + std::to_string (i) + " Release Curve", Normalize (-1.0f, 1.0f, 0.01f), 0.25f));
    }

    // ================================================================================================================================================
    // Velocity & Keyboard Curve Parameters
    layout.add (make_unique<Parameter> (ParameterID ("velocityCurve", 1), "Velocity Curve", Normalize (-1.0f, 1.0f, 0.01f), 0.0f));
    layout.add (make_unique<Parameter> (ParameterID ("keyboardCurve", 1), "Keyboard Curve", Normalize (-1.0f, 1.0f, 0.01f), 0.0f));

    return layout;
}
