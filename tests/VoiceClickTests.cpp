#include <PluginProcessor.h>
#include <Synthesizer/MySynth.h>
#include <catch2/catch_test_macros.hpp>

namespace
{
constexpr double kSampleRate = 44100.0;
constexpr int kBlockSize = 512;

struct ClickFixture
{
    ClickFixture()
    {
        plugin.prepareToPlay (kSampleRate, kBlockSize);
    }

    void setParam (const char* id, float normalizedValue)
    {
        plugin.parameters.getParameter (id)->setValueNotifyingHost (normalizedValue);
        plugin.getSynth().updateParameters ({});
    }

    // Renders one block and returns it for inspection.
    juce::AudioBuffer<float> renderBlock()
    {
        juce::AudioBuffer<float> buffer (2, kBlockSize);
        buffer.clear();
        juce::MidiBuffer midi;
        plugin.getSynth().renderNextBlock (buffer, midi, 0, kBlockSize);
        return buffer;
    }

    static float peak (const juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
    {
        return buffer.getMagnitude (startSample, numSamples);
    }

    PluginProcessor plugin;
};
} // namespace

// Parameters changed while a voice is idle (e.g. a preset restored after the
// plugin is constructed) must be picked up before the voice's first rendered
// block. The cached per-voice values are only refreshed while rendering, so
// without an explicit sync in startNote the first block plays with stale
// values and the jump to the live ones clicks.
TEST_CASE ("Note start uses live parameter values, not stale cached ones", "[voices][clicks]")
{
    ClickFixture fx;

    // Simulate a preset load after voice construction: osc level drops from
    // its default of 1.0 to 0.0 while every voice is idle.
    fx.setParam (ParamIDs::mainOscLevel, 0.0f);

    fx.plugin.getSynth().noteOn (1, 60, 0.8f);
    const auto firstBlock = fx.renderBlock();

    // With the level at zero the very first block must already be silent.
    CHECK (ClickFixture::peak (firstBlock, 0, kBlockSize) < 1.0e-6f);
}

// After a voice is hard-stopped (voice steal cleanup, allNotesOff) its output
// is cut to silence, so the slewed envelope value must be reset — otherwise
// the next note on that voice starts at the leftover envelope level instead
// of attacking from zero, which clicks.
TEST_CASE ("Retriggered voice attacks from silence after a hard stop", "[voices][clicks]")
{
    ClickFixture fx;
    auto& synth = fx.plugin.getSynth();

    // Establish a note and let the envelope slew reach full level.
    synth.noteOn (1, 60, 1.0f);
    fx.renderBlock();
    const auto established = fx.renderBlock();
    const float establishedPeak = ClickFixture::peak (established, 0, kBlockSize);
    REQUIRE (establishedPeak > 0.01f);

    // Hard stop (no tail-off), then render a silent block.
    synth.allNotesOff (0, false);
    const auto silentBlock = fx.renderBlock();
    REQUIRE (ClickFixture::peak (silentBlock, 0, kBlockSize) < 1.0e-6f);

    // Retrigger the same voice. Coming out of silence, the first samples must
    // ramp up from zero rather than resume at the previous envelope level.
    synth.noteOn (1, 60, 1.0f);
    const auto retriggered = fx.renderBlock();
    const float startPeak = ClickFixture::peak (retriggered, 0, 32);
    CHECK (startPeak < 0.5f * establishedPeak);
}
