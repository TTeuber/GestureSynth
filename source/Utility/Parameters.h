//
// Created by Tyler Teuber on 5/10/25.
//

#pragma once

#include "juce_audio_processors/juce_audio_processors.h"

namespace ModDest
{
    inline constexpr const char* paramIDs[] = {
        "filterFrequency",  // 0
        "filterResonance",  // 1
        "hpfFrequency",     // 2
        "fineTune",         // 3
        "pulseWidth",       // 4
        "oscWaveform",      // 5
        "oscDetune",        // 6
        "oscWidth",         // 7
        "subOsc",           // 8
        "subOscWave",       // 9
        "vibratoDepth",     // 10
        "vibratoRate",      // 11
        "chorusDepth",      // 12
        "chorusRate",       // 13
        "mainOscLevel",     // 14
        "noiseLevel",       // 15
        "noiseTone",        // 16
    };

    inline constexpr int count = static_cast<int> (sizeof (paramIDs) / sizeof (paramIDs[0]));

    inline constexpr int filterCutoff    = 0;
    inline constexpr int filterResonance = 1;
    inline constexpr int hpfCutoff       = 2;
    inline constexpr int fineTune        = 3;
    inline constexpr int pulseWidth      = 4;
    inline constexpr int oscWaveform     = 5;
    inline constexpr int oscDetune       = 6;
    inline constexpr int oscWidth        = 7;
    inline constexpr int subOsc          = 8;
    inline constexpr int subOscWave      = 9;
    inline constexpr int vibratoDepth    = 10;
    inline constexpr int vibratoRate     = 11;
    inline constexpr int chorusDepth     = 12;
    inline constexpr int chorusRate      = 13;
    inline constexpr int mainOscLevel    = 14;
    inline constexpr int noiseLevel      = 15;
    inline constexpr int noiseTone       = 16;

    inline juce::StringArray buildDestIDs()
    {
        juce::StringArray result;
        result.add ("None");
        for (int i = 0; i < count; ++i)
            result.add (paramIDs[i]);
        return result;
    }
}

namespace ParamIDs
{
    // Global
    inline constexpr auto volume       = "volume";
    inline constexpr auto noiseLevel   = "noiseLevel";
    inline constexpr auto noiseTone    = "noiseTone";

    // Filter
    inline constexpr auto filterFrequency  = "filterFrequency";
    inline constexpr auto filterResonance  = "filterResonance";
    inline constexpr auto hpfFrequency     = "hpfFrequency";

    // Oscillator
    inline constexpr auto oscWaveform  = "oscWaveform";
    inline constexpr auto pulseWidth   = "pulseWidth";
    inline constexpr auto oscDetune    = "oscDetune";
    inline constexpr auto oscWidth     = "oscWidth";
    inline constexpr auto subOsc       = "subOsc";
    inline constexpr auto subOscWave   = "subOscWave";
    inline constexpr auto mainOscLevel = "mainOscLevel";
    inline constexpr auto fineTune     = "fineTune";

    // Chorus
    inline constexpr auto chorusMix    = "chorusMix";
    inline constexpr auto chorusDepth  = "chorusDepth";
    inline constexpr auto chorusRate   = "chorusRate";

    // Vibrato
    inline constexpr auto vibratoRate  = "vibratoRate";
    inline constexpr auto vibratoDepth = "vibratoDepth";

    // LFO (use lfoParamID helper below for indexed access)
    inline constexpr auto manualBpm    = "manualBpm";

    // Boolean toggles
    inline constexpr auto oscOn        = "oscOn";
    inline constexpr auto detuneOn     = "detuneOn";
    inline constexpr auto subOn        = "subOn";
    inline constexpr auto filterOn     = "filterOn";
    inline constexpr auto hpfOn        = "hpfOn";
    inline constexpr auto noiseOn      = "noiseOn";
    inline constexpr auto chorusOn     = "chorusOn";
    inline constexpr auto vibratoOn    = "vibratoOn";
    inline constexpr auto delayOn      = "delayOn";
    inline constexpr auto reverbOn     = "reverbOn";

    // Portamento
    inline constexpr auto portamentoTime = "portamentoTime";

    // Reverb
    inline constexpr auto reverbDecay      = "reverbDecay";
    inline constexpr auto reverbSize       = "reverbSize";
    inline constexpr auto reverbDamping    = "reverbDamping";
    inline constexpr auto reverbBassMult   = "reverbBassMult";
    inline constexpr auto reverbModRate    = "reverbModRate";
    inline constexpr auto reverbModDepth   = "reverbModDepth";
    inline constexpr auto reverbDiffusion  = "reverbDiffusion";
    inline constexpr auto reverbPreDelay   = "reverbPreDelay";
    inline constexpr auto reverbWidth      = "reverbWidth";
    inline constexpr auto reverbLevel      = "reverbLevel";

    // Delay
    inline constexpr auto delayTime         = "delayTime";
    inline constexpr auto delayTempoSync    = "delayTempoSync";
    inline constexpr auto delayNoteDivision = "delayNoteDivision";
    inline constexpr auto delayFeedback     = "delayFeedback";
    inline constexpr auto delayMix          = "delayMix";
    inline constexpr auto delayLowpass      = "delayLowpass";
    inline constexpr auto delayHighpass     = "delayHighpass";
    inline constexpr auto delaySaturation   = "delaySaturation";
    inline constexpr auto delayModRate      = "delayModRate";
    inline constexpr auto delayModDepth     = "delayModDepth";
    inline constexpr auto delayPingPong     = "delayPingPong";
    inline constexpr auto delayDiffusion    = "delayDiffusion";

    // Voice mode
    inline constexpr auto monoOn         = "monoOn";
    inline constexpr auto legatoOn       = "legatoOn";
    inline constexpr auto pitchBendRange = "pitchBendRange";
    inline constexpr auto voiceCount     = "voiceCount";

    // Velocity & Keyboard curves
    inline constexpr auto velocityCurve  = "velocityCurve";
    inline constexpr auto keyboardCurve  = "keyboardCurve";

    // Envelope helper: returns e.g. "env1Attack", "env2DecayCurve"
    inline juce::String envParamID (int envIndex, const char* suffix)
    {
        return "env" + juce::String (envIndex) + suffix;
    }

    // LFO helper: returns e.g. "lfo1Rate", "lfo2TempoSync"
    inline juce::String lfoParamID (int lfoIndex, const char* suffix)
    {
        return "lfo" + juce::String (lfoIndex) + suffix;
    }
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    using namespace std;
    using namespace juce;
    using Parameter = AudioParameterFloat;
    using Normalize = NormalisableRange<float>;
    using ID = ParameterID;

    // ================================================================================================================================================
    // Global Parameters
    layout.add (make_unique<Parameter> (ID (ParamIDs::volume, 1), "Volume", 0.0f, 1.0f, 0.5f));
    layout.add (make_unique<Parameter> (ID (ParamIDs::noiseLevel, 1), "Noise", 0.0f, 1.0f, 0.0f));
    layout.add (make_unique<Parameter> (ID (ParamIDs::noiseTone, 1), "Noise Tone", 0.0f, 1.0f, 0.5f));

    // ================================================================================================================================================
    // Filter Parameters
    auto filterFrequency = make_unique<Parameter> (ID (ParamIDs::filterFrequency, 1), "Filter Frequency", Normalize (20.f, 20000.f, 0.01f, 0.25f), 20000.0f);
    filterFrequency->range.setSkewForCentre (1200.0f);
    layout.add (std::move (filterFrequency));

    auto filterResonance = make_unique<Parameter> (ID (ParamIDs::filterResonance, 1), "Filter Resonance", Normalize (0.1, 10, 0.01, 0.45), 0.71f);
    filterResonance->range.setSkewForCentre (1.0f);
    layout.add (std::move (filterResonance));

    auto hpfFrequency = make_unique<Parameter> (ID (ParamIDs::hpfFrequency, 1), "HPF Frequency", Normalize (20.f, 3000.f, 0.01f, 0.25f), 20.0f);
    hpfFrequency->range.setSkewForCentre (300.0f);
    layout.add (std::move (hpfFrequency));

    // ================================================================================================================================================
    // Oscillator Parameters
    layout.add (make_unique<Parameter> (ID (ParamIDs::oscWaveform, 1), "Waveform", 0.0f, 1.0f, 0.5f));
    layout.add (make_unique<Parameter> (ID (ParamIDs::pulseWidth, 1), "Pulse Width", 0.1f, 0.9f, 0.5f));
    layout.add (make_unique<Parameter> (ID (ParamIDs::oscDetune, 1), "Detune", 0.0f, 1.0f, 0.2f));
    layout.add (make_unique<Parameter> (ID (ParamIDs::oscWidth, 1), "Width", 0.0f, 1.0f, 1.0f));
    layout.add (make_unique<Parameter> (ID (ParamIDs::subOsc, 1), "Sub Level", 0.0f, 1.0f, 0.75f));
    layout.add (make_unique<Parameter> (ID (ParamIDs::subOscWave, 2), "Sub Wave", 0.0f, 1.0f, 0.67f));
    layout.add (make_unique<Parameter> (ID (ParamIDs::mainOscLevel, 1), "Osc Level", 0.0f, 1.0f, 1.0f));
    layout.add (make_unique<Parameter> (ID (ParamIDs::fineTune, 1), "Fine Tune", -0.5f, 0.5f, 0.0f));

    // ================================================================================================================================================
    // Chorus Parameters
    layout.add (make_unique<Parameter> (ID (ParamIDs::chorusMix, 1), "Chrs Mix", 0.0f, 1.0f, 0.5f));
    layout.add (make_unique<Parameter> (ID (ParamIDs::chorusDepth, 1), "Chorus Depth", Normalize (0.0f, 0.03f, 0.0f), 0.015f));
    auto chorusRate = make_unique<Parameter> (ID (ParamIDs::chorusRate, 1), "Chorus Rate", Normalize (0.0f, 1.0f, 0.0f), 0.15f);
    chorusRate->range.setSkewForCentre (0.3f);
    layout.add (std::move (chorusRate));

    // ================================================================================================================================================
    // Vibrato Parameters
    auto vibratoRate = make_unique<Parameter> (ID (ParamIDs::vibratoRate, 1), "Vibrato Rate",
        Normalize (0.1f, 20.0f, 0.01f, 0.35f), 5.0f);
    vibratoRate->range.setSkewForCentre (5.0f);
    layout.add (std::move (vibratoRate));
    layout.add (make_unique<Parameter> (ID (ParamIDs::vibratoDepth, 1), "Vibrato Depth",
        Normalize (0.0f, 2.0f, 0.001f), 0.0f));

    // ================================================================================================================================================
    // LFO Parameters
    for (int i = 1; i <= 4; i++)
    {
        auto lfoRate = make_unique<Parameter> (ID (ParamIDs::lfoParamID (i, "Rate"), 1), "LFO " + std::to_string (i) + " Rate", Normalize (0.01f, 20.0f, 0.001f, 0.35f), 1.0f);
        lfoRate->range.setSkewForCentre (2.0f);
        layout.add (std::move (lfoRate));

        layout.add (make_unique<AudioParameterBool> (ID (ParamIDs::lfoParamID (i, "TempoSync"), 1), "LFO " + std::to_string (i) + " Tempo Sync", false));
        layout.add (make_unique<AudioParameterChoice> (ID (ParamIDs::lfoParamID (i, "NoteDivision"), 1), "LFO " + std::to_string (i) + " Note Division",
            juce::StringArray { "4/1", "2/1", "1/1", "1/2", "1/2T", "1/2D", "1/4", "1/4T", "1/4D", "1/8", "1/8T", "1/8D", "1/16", "1/16T", "1/16D", "1/32" }, 6));
        layout.add (make_unique<AudioParameterBool> (ID (ParamIDs::lfoParamID (i, "BeatSync"), 1), "LFO " + std::to_string (i) + " Beat Sync", false));
        layout.add (make_unique<AudioParameterBool> (ID (ParamIDs::lfoParamID (i, "Mono"), 1), "LFO " + std::to_string (i) + " Mono", false));
    }

    auto manualBpm = make_unique<Parameter> (ID (ParamIDs::manualBpm, 1), "Manual BPM", Normalize (20.0f, 300.0f, 0.1f), 120.0f);
    layout.add (std::move (manualBpm));

    // ================================================================================================================================================
    // Boolean Parameters
    layout.add (make_unique<AudioParameterBool> (ID (ParamIDs::oscOn, 1), "Osc On", true));
    layout.add (make_unique<AudioParameterBool> (ID (ParamIDs::detuneOn, 1), "Detune On", false));
    layout.add (make_unique<AudioParameterBool> (ID (ParamIDs::subOn, 1), "Sub On", false));
    layout.add (make_unique<AudioParameterBool> (ID (ParamIDs::filterOn, 1), "Filter On", true));
    layout.add (make_unique<AudioParameterBool> (ID (ParamIDs::hpfOn, 1), "HPF On", true));
    layout.add (make_unique<AudioParameterBool> (ID (ParamIDs::noiseOn, 1), "Noise On", false));
    layout.add (make_unique<AudioParameterBool> (ID (ParamIDs::chorusOn, 1), "Chorus On", false));
    layout.add (make_unique<AudioParameterBool> (ID (ParamIDs::vibratoOn, 1), "Vibrato On", true));
    layout.add (make_unique<AudioParameterBool> (ID (ParamIDs::delayOn, 1), "Delay On", false));
    layout.add (make_unique<AudioParameterBool> (ID (ParamIDs::reverbOn, 1), "Reverb On", false));

    // ================================================================================================================================================
    // Portamento Parameters
    auto portamentoTime = make_unique<Parameter> (ID (ParamIDs::portamentoTime, 1), "Porta", Normalize (0.0f, 5000.0f, 0.1f), 0.0f);
    portamentoTime->range.setSkewForCentre (500.0f);
    layout.add (std::move (portamentoTime));

    // ================================================================================================================================================
    // Reverb Parameters
    layout.add (std::make_unique<Parameter> (ID (ParamIDs::reverbDecay, 1), "Decay",
        Normalize (1.0f, 20.0f, 0.1f, 0.2f), 4.0f));
    layout.add (std::make_unique<Parameter> (ID (ParamIDs::reverbSize, 1), "Size",
        Normalize (1.0f, 4.0f, 0.1f), 2.0f));
    layout.add (std::make_unique<Parameter> (ID (ParamIDs::reverbDamping, 1), "Damping",
        Normalize (1000.0f, 16000.0f, 10.0f, 0.5f), 6000.0f));
    layout.add (std::make_unique<Parameter> (ID (ParamIDs::reverbBassMult, 1), "Bass Mult",
        Normalize (0.5f, 2.0f, 0.01f), 1.5f));
    layout.add (std::make_unique<Parameter> (ID (ParamIDs::reverbModRate, 1), "Rate",
        Normalize (0.1f, 5.0f, 0.01f, 0.5f), 1.0f));
    layout.add (std::make_unique<Parameter> (ID (ParamIDs::reverbModDepth, 1), "Depth",
        Normalize (0.0f, 1.0f, 0.01f), 0.3f));
    layout.add (std::make_unique<Parameter> (ID (ParamIDs::reverbDiffusion, 1), "Reverb Diffusion",
        Normalize (0.0f, 1.0f, 0.01f), 0.7f));
    layout.add (std::make_unique<Parameter> (ID (ParamIDs::reverbPreDelay, 1), "Pre-Delay",
        Normalize (0.0f, 300.0f, 1.0f), 20.0f));
    layout.add (std::make_unique<Parameter> (ID (ParamIDs::reverbWidth, 1), "Reverb Width",
        Normalize (0.0f, 1.0f, 0.01f), 1.0f));
    layout.add (std::make_unique<Parameter> (ID (ParamIDs::reverbLevel, 1), "Level",
        Normalize (0.0f, 1.0f, 0.01f), 0.35f));

    // ================================================================================================================================================
    // Delay Parameters
    auto delayTime = make_unique<Parameter> (ID (ParamIDs::delayTime, 1), "Time",
        Normalize (10.0f, 2000.0f, 0.1f, 0.35f), 375.0f);
    layout.add (std::move (delayTime));
    layout.add (make_unique<AudioParameterBool> (ID (ParamIDs::delayTempoSync, 1), "Delay Tempo Sync", false));
    layout.add (make_unique<AudioParameterChoice> (ID (ParamIDs::delayNoteDivision, 1), "Delay Note Division",
        juce::StringArray { "1/1", "1/2", "1/2T", "1/2D", "1/4", "1/4T", "1/4D", "1/8", "1/8T", "1/8D", "1/16", "1/16T", "1/16D" }, 4));
    layout.add (make_unique<Parameter> (ID (ParamIDs::delayFeedback, 1), "Feedback",
        Normalize (0.0f, 0.95f, 0.01f), 0.4f));
    layout.add (make_unique<Parameter> (ID (ParamIDs::delayMix, 1), "Mix",
        Normalize (0.0f, 0.5f, 0.01f), 0.3f));
    auto delayLowpass = make_unique<Parameter> (ID (ParamIDs::delayLowpass, 1), "Lowpass",
        Normalize (200.0f, 18000.0f, 1.0f, 0.35f), 4000.0f);
    layout.add (std::move (delayLowpass));
    auto delayHighpass = make_unique<Parameter> (ID (ParamIDs::delayHighpass, 1), "Highpass",
        Normalize (20.0f, 2000.0f, 0.1f, 0.35f), 80.0f);
    layout.add (std::move (delayHighpass));
    layout.add (make_unique<Parameter> (ID (ParamIDs::delaySaturation, 1), "Delay Saturation",
        Normalize (0.0f, 1.0f, 0.01f), 0.3f));
    auto delayModRate = make_unique<Parameter> (ID (ParamIDs::delayModRate, 1), "Rate",
        Normalize (0.05f, 5.0f, 0.01f, 0.5f), 0.7f);
    layout.add (std::move (delayModRate));
    layout.add (make_unique<Parameter> (ID (ParamIDs::delayModDepth, 1), "Depth",
        Normalize (0.0f, 1.0f, 0.01f), 0.15f));
    layout.add (make_unique<AudioParameterBool> (ID (ParamIDs::delayPingPong, 1), "Delay Ping Pong", false));
    layout.add (make_unique<Parameter> (ID (ParamIDs::delayDiffusion, 1), "Delay Diffusion",
        Normalize (0.0f, 1.0f, 0.01f), 0.0f));

    // ================================================================================================================================================
    // Voice Mode Parameters
    layout.add (make_unique<AudioParameterBool> (ID (ParamIDs::monoOn, 1), "Mono Mode", false));
    layout.add (make_unique<AudioParameterBool> (ID (ParamIDs::legatoOn, 1), "Legato", false));
    layout.add (make_unique<Parameter> (ID (ParamIDs::pitchBendRange, 1), "Pitch Bend Range", Normalize (1.0f, 12.0f, 1.0f), 2.0f));
    layout.add (make_unique<AudioParameterChoice> (ID (ParamIDs::voiceCount, 1), "Voice Count",
        juce::StringArray { "2", "3", "4", "5", "6", "8", "12", "16" }, 5));

    // ================================================================================================================================================
    // Envelope Parameters

    for (int i = 1; i <= 4; i++)
    {
        layout.add (make_unique<Parameter> (ID (ParamIDs::envParamID (i, "Attack"), 1), "Envelope " + std::to_string (i) + " Attack", Normalize (0.01f, 10.0f, 0.001f, 0.3f), 0.0f));
        layout.add (make_unique<Parameter> (ID (ParamIDs::envParamID (i, "AttackCurve"), 1), "Envelope " + std::to_string (i) + " Attack Curve", Normalize (-1.0f, 1.0f, 0.01f), 0.0f));
        layout.add (make_unique<Parameter> (ID (ParamIDs::envParamID (i, "Decay"), 1), "Envelope " + std::to_string (i) + " Decay", Normalize (0.01f, 10.0f, 0.001f, 0.3f), 0.5f));
        layout.add (make_unique<Parameter> (ID (ParamIDs::envParamID (i, "DecayCurve"), 1), "Envelope " + std::to_string (i) + " Decay Curve", Normalize (-1.0f, 1.0f, 0.01f), 0.25f));
        layout.add (make_unique<Parameter> (ID (ParamIDs::envParamID (i, "Sustain"), 1), "Envelope " + std::to_string (i) + " Sustain", 0.0f, 1.0f, 1.0f));
        layout.add (make_unique<Parameter> (ID (ParamIDs::envParamID (i, "Release"), 1), "Envelope " + std::to_string (i) + " Release", Normalize (0.01f, 10.0f, 0.001f, 0.3f), 0.0f));
        layout.add (make_unique<Parameter> (ID (ParamIDs::envParamID (i, "ReleaseCurve"), 1), "Envelope " + std::to_string (i) + " Release Curve", Normalize (-1.0f, 1.0f, 0.01f), 0.25f));
    }

    // ================================================================================================================================================
    // Velocity & Keyboard Curve Parameters
    layout.add (make_unique<Parameter> (ID (ParamIDs::velocityCurve, 1), "Velocity Curve", Normalize (-1.0f, 1.0f, 0.01f), 0.0f));
    layout.add (make_unique<Parameter> (ID (ParamIDs::keyboardCurve, 1), "Keyboard Curve", Normalize (-1.0f, 1.0f, 0.01f), 0.0f));

    return layout;
}
