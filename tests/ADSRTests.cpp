#include <Modulation/MyADSR.h>
#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

namespace
{
constexpr float kSampleRate = 48000.0f;

// MyADSR::setParameters takes normalized (0-1) values, since StaticParameter
// converts through the parameter's NormalisableRange on read.
MyADSR::Parameters makeParams (const juce::AudioProcessorValueTreeState& apvts,
                               float attackSeconds, float decaySeconds,
                               float sustainLevel, float releaseSeconds)
{
    auto normalize = [&apvts] (const char* suffix, float seconds)
    {
        return apvts.getParameter (ParamIDs::envParamID (1, suffix))
            ->getNormalisableRange()
            .convertTo0to1 (seconds);
    };

    // Curve parameters live on a -1..1 range, so normalized 0.5 == linear.
    return { normalize ("Attack", attackSeconds),
             normalize ("Decay", decaySeconds),
             sustainLevel, // sustain range is already 0-1 linear
             normalize ("Release", releaseSeconds),
             0.5f, 0.5f, 0.5f };
}

int samplesFor (float seconds) { return static_cast<int> (seconds * kSampleRate); }

float advance (MyADSR& env, int numSamples)
{
    float value = env.getCurrentValue();
    for (int i = 0; i < numSamples; ++i)
        value = env.getNextValue();
    return value;
}
} // namespace

TEST_CASE ("ADSR stages hit the expected values at the expected times", "[adsr]")
{
    PluginProcessor plugin;
    MyADSR env (plugin.parameters, 1);
    env.setSampleRate (kSampleRate);
    env.setParameters (makeParams (plugin.parameters, 0.1f, 0.2f, 0.5f, 0.1f));

    REQUIRE_FALSE (env.isActive());
    env.noteOn();
    REQUIRE (env.isActive());

    SECTION ("attack ramps linearly to 1.0 over the attack time")
    {
        // Curve parameter defaults to 0 (linear), so halfway through the
        // attack the envelope should be at ~0.5.
        const float midAttack = advance (env, samplesFor (0.05f));
        CHECK_THAT (midAttack, WithinAbs (0.5f, 0.02f));

        const float endAttack = advance (env, samplesFor (0.05f) + 2);
        CHECK_THAT (endAttack, WithinAbs (1.0f, 0.02f));
    }

    SECTION ("decay falls from 1.0 to the sustain level over the decay time")
    {
        advance (env, samplesFor (0.1f) + 2); // complete the attack

        const float midDecay = advance (env, samplesFor (0.1f));
        CHECK_THAT (midDecay, WithinAbs (0.75f, 0.02f)); // halfway between 1.0 and 0.5

        const float endDecay = advance (env, samplesFor (0.1f) + 2);
        CHECK_THAT (endDecay, WithinAbs (0.5f, 0.02f));
    }

    SECTION ("sustain holds indefinitely while the note is on")
    {
        advance (env, samplesFor (0.3f) + 4); // attack + decay
        const float sustained = advance (env, samplesFor (1.0f));
        CHECK_THAT (sustained, WithinAbs (0.5f, 0.001f));
        CHECK (env.isActive());
    }

    SECTION ("release falls from the sustain level to zero over the release time")
    {
        advance (env, samplesFor (0.3f) + 4); // reach sustain
        env.noteOff();

        const float midRelease = advance (env, samplesFor (0.05f));
        CHECK_THAT (midRelease, WithinAbs (0.25f, 0.02f)); // half of sustain 0.5

        advance (env, samplesFor (0.05f) + 2);
        CHECK_FALSE (env.isActive());
        CHECK_THAT (env.getCurrentValue(), WithinAbs (0.0f, 0.001f));
    }
}

TEST_CASE ("Note-off during the attack releases from the current level", "[adsr]")
{
    PluginProcessor plugin;
    MyADSR env (plugin.parameters, 1);
    env.setSampleRate (kSampleRate);
    env.setParameters (makeParams (plugin.parameters, 0.1f, 0.2f, 0.5f, 0.1f));

    env.noteOn();
    const float atRelease = advance (env, samplesFor (0.05f)); // mid-attack, ~0.5
    env.noteOff();

    // The release should start from the interrupted level, not jump to sustain.
    const float justAfter = advance (env, 16);
    CHECK (justAfter <= atRelease);
    CHECK (justAfter > 0.25f);

    advance (env, samplesFor (0.1f) + 2);
    CHECK_FALSE (env.isActive());
}

TEST_CASE ("Envelope retriggers cleanly after a full cycle", "[adsr]")
{
    PluginProcessor plugin;
    MyADSR env (plugin.parameters, 1);
    env.setSampleRate (kSampleRate);
    env.setParameters (makeParams (plugin.parameters, 0.1f, 0.2f, 0.5f, 0.1f));

    env.noteOn();
    advance (env, samplesFor (0.4f));
    env.noteOff();
    advance (env, samplesFor (0.2f));
    REQUIRE_FALSE (env.isActive());

    env.noteOn();
    REQUIRE (env.isActive());
    const float midAttack = advance (env, samplesFor (0.05f));
    CHECK_THAT (midAttack, WithinAbs (0.5f, 0.02f));
}
