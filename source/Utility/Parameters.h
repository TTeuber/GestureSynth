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

    // ================================================================================================================================================
    // Filter Parameters
    auto filterFrequency = make_unique<Parameter> (ParameterID ("filterFrequency", 1), "Filter Frequency", Normalize (20.f, 20000.f, 0.01f, 0.25f), 20000.0f);
    filterFrequency->range.setSkewForCentre (1200.0f);
    layout.add (std::move (filterFrequency));

    auto filterResonance = make_unique<Parameter> (ParameterID ("filterResonance", 1), "Filter Resonance", Normalize (0.1, 10, 0.01, 0.45), 0.71f);
    filterResonance->range.setSkewForCentre (1.0f);
    layout.add (std::move (filterResonance));

    // ================================================================================================================================================
    // Oscillator Parameters
    layout.add (make_unique<Parameter> (ParameterID ("oscWaveform", 1), "Waveform", 0.0f, 1.0f, 0.5f));
    layout.add (make_unique<Parameter> (ParameterID ("pulseWidth", 1), "Pulse Width", 0.1f, 0.9f, 0.5f));
    layout.add (make_unique<Parameter> (ParameterID ("oscDetune", 1), "Detune", 0.0f, 1.0f, 0.0f));
    layout.add (make_unique<Parameter> (ParameterID ("oscWidth", 1), "Width", 0.0f, 1.0f, 1.0f));
    layout.add (make_unique<Parameter> (ParameterID ("subOsc", 1), "Sub Level", 0.0f, 1.0f, 0.0f));
    layout.add (make_unique<Parameter> (ParameterID ("subOscWave", 1), "Sub Wave", Normalize (1, 4, 1), 1.0f));
    layout.add (make_unique<Parameter> (ParameterID ("fineTune", 1), "Fine Tune", -0.5f, 0.5f, 0.0f));

    // ================================================================================================================================================
    // Chorus Parameters
    layout.add (make_unique<Parameter> (ParameterID ("chorusMix", 1), "Chorus Mix", 0.0f, 1.0f, 0.5f));
    layout.add (make_unique<Parameter> (ParameterID ("chorusDepth", 1), "Chorus Depth", Normalize (0.0f, 0.03f, 0.0f), 0.0028f));
    auto chorusRate = make_unique<Parameter> (ParameterID ("chorusRate", 1), "Chorus Rate", Normalize (0.0f, 1.0f, 0.0f), 0.3f);
    chorusRate->range.setSkewForCentre (0.3f);
    layout.add (std::move (chorusRate));

    // ================================================================================================================================================
    // LFO Parameters
    auto lfo1Rate = make_unique<Parameter> (ParameterID ("lfo1Rate", 1), "LFO 1 Rate", Normalize (0.01f, 20.0f, 0.001f, 0.35f), 1.0f);
    lfo1Rate->range.setSkewForCentre (2.0f);
    layout.add (std::move (lfo1Rate));

    // ================================================================================================================================================
    // Boolean Parameters
    layout.add (make_unique<AudioParameterBool> (ParameterID ("oscOn", 1), "Osc On", true));
    layout.add (make_unique<AudioParameterBool> (ParameterID ("detuneOn", 1), "Detune On", true));
    layout.add (make_unique<AudioParameterBool> (ParameterID ("subOn", 1), "Sub On", true));
    layout.add (make_unique<AudioParameterBool> (ParameterID ("filterOn", 1), "Filter On", true));
    layout.add (make_unique<AudioParameterBool> (ParameterID ("chorusOn", 1), "Chorus On", true));

    // ================================================================================================================================================
    // Envelope Parameters

    for (int i = 1; i < 2; i++)
    {
        layout.add (make_unique<Parameter> (ParameterID ("env" + std::to_string (i) + "Attack", 1), "Envelope " + std::to_string (i) + " Attack", Normalize (0.01f, 10.0f, 0.001f, 0.3f), 0.0f));
        layout.add (make_unique<Parameter> (ParameterID ("env" + std::to_string (i) + "AttackCurve", 1), "Envelope " + std::to_string (i) + " Attack Curve", Normalize (0.1f, 10.0f, 0.001f, 0.3f), 1.0f));
        layout.add (make_unique<Parameter> (ParameterID ("env" + std::to_string (i) + "Decay", 1), "Envelope " + std::to_string (i) + " Decay", Normalize (0.01f, 10.0f, 0.001f, 0.3f), 0.5f));
        layout.add (make_unique<Parameter> (ParameterID ("env" + std::to_string (i) + "DecayCurve", 1), "Envelope " + std::to_string (i) + " Decay Curve", Normalize (0.1f, 10.0f, 0.001f, 0.3f), 1.0f));
        layout.add (make_unique<Parameter> (ParameterID ("env" + std::to_string (i) + "Sustain", 1), "Envelope " + std::to_string (i) + " Sustain", 0.0f, 1.0f, 1.0f));
        layout.add (make_unique<Parameter> (ParameterID ("env" + std::to_string (i) + "Release", 1), "Envelope " + std::to_string (i) + " Release", Normalize (0.01f, 10.0f, 0.001f, 0.3f), 0.0f));
        layout.add (make_unique<Parameter> (ParameterID ("env" + std::to_string (i) + "ReleaseCurve", 1), "Envelope " + std::to_string (i) + " Release Curve", Normalize (0.1f, 10.0f, 0.001f, 0.3f), 1.0f));
    }

    return layout;
}
