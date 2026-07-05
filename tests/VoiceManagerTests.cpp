#include <PluginProcessor.h>
#include <Synthesizer/MySynth.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinRel;

namespace
{
constexpr double kSampleRate = 44100.0;
constexpr int kBlockSize = 512;

struct SynthFixture
{
    SynthFixture()
    {
        plugin.prepareToPlay (kSampleRate, kBlockSize);
    }

    void setParam (const char* id, float normalizedValue)
    {
        plugin.parameters.getParameter (id)->setValueNotifyingHost (normalizedValue);
        plugin.getSynth().updateParameters ({});
    }

    void enableMono (bool legato = false)
    {
        setParam (ParamIDs::monoOn, 1.0f);
        setParam (ParamIDs::legatoOn, legato ? 1.0f : 0.0f);
    }

    int activeVoiceCount()
    {
        auto& synth = plugin.getSynth();
        int count = 0;
        for (int i = 0; i < synth.getNumVoices(); ++i)
            if (synth.getVoice (i)->isVoiceActive())
                ++count;
        return count;
    }

    int playingNote() { return plugin.getSynth().getVoice (0)->getCurrentlyPlayingNote(); }

    float trackedFrequency() { return plugin.pitchTracker->frequency; }

    // Render enough audio for envelope release tails to finish so voices free up.
    void renderSeconds (double seconds)
    {
        juce::AudioBuffer<float> buffer (2, kBlockSize);
        juce::MidiBuffer midi;
        const int numBlocks = static_cast<int> (seconds * kSampleRate / kBlockSize) + 1;
        for (int i = 0; i < numBlocks; ++i)
        {
            buffer.clear();
            plugin.getSynth().renderNextBlock (buffer, midi, 0, kBlockSize);
        }
    }

    PluginProcessor plugin;
};

float noteHz (int midiNote)
{
    return static_cast<float> (juce::MidiMessage::getMidiNoteInHertz (midiNote));
}
} // namespace

TEST_CASE ("Poly mode plays one voice per held note", "[voices]")
{
    SynthFixture fx;
    auto& synth = fx.plugin.getSynth();

    synth.noteOn (1, 60, 0.8f);
    synth.noteOn (1, 64, 0.8f);
    synth.noteOn (1, 67, 0.8f);
    CHECK (fx.activeVoiceCount() == 3);

    synth.noteOff (1, 64, 0.0f, false);
    CHECK (fx.activeVoiceCount() == 2);
}

TEST_CASE ("Mono mode allows only one active voice", "[voices]")
{
    SynthFixture fx;
    fx.enableMono();
    auto& synth = fx.plugin.getSynth();

    synth.noteOn (1, 60, 0.8f);
    synth.noteOn (1, 64, 0.8f);
    synth.noteOn (1, 67, 0.8f);

    CHECK (fx.activeVoiceCount() == 1);
    CHECK (fx.playingNote() == 67);
}

TEST_CASE ("Mono mode returns to the most recent held note on release", "[voices]")
{
    SynthFixture fx;
    fx.enableMono();
    auto& synth = fx.plugin.getSynth();

    synth.noteOn (1, 60, 0.8f);
    synth.noteOn (1, 64, 0.8f);
    synth.noteOn (1, 67, 0.8f);
    REQUIRE (fx.playingNote() == 67);

    synth.noteOff (1, 67, 0.0f, true);
    CHECK (fx.playingNote() == 64);

    synth.noteOff (1, 64, 0.0f, true);
    CHECK (fx.playingNote() == 60);

    synth.noteOff (1, 60, 0.0f, true);
    fx.renderSeconds (1.0);
    CHECK (fx.activeVoiceCount() == 0);
}

TEST_CASE ("Switching to mono mode silences stacked poly voices", "[voices]")
{
    SynthFixture fx;
    auto& synth = fx.plugin.getSynth();

    synth.noteOn (1, 60, 0.8f);
    synth.noteOn (1, 64, 0.8f);
    REQUIRE (fx.activeVoiceCount() == 2);

    fx.enableMono();
    synth.noteOn (1, 67, 0.8f);

    CHECK (fx.activeVoiceCount() == 1);
    CHECK (fx.playingNote() == 67);
}

TEST_CASE ("Legato glides to new notes without retriggering", "[voices]")
{
    SynthFixture fx;
    fx.enableMono (true);
    auto& synth = fx.plugin.getSynth();

    synth.noteOn (1, 60, 0.8f);
    REQUIRE (fx.playingNote() == 60);
    REQUIRE_THAT (fx.trackedFrequency(), WithinRel (noteHz (60), 1e-4f));

    // Second overlapping note glides: pitch moves to the new note but the
    // voice is not restarted (the playing note stays as originally triggered).
    synth.noteOn (1, 64, 0.8f);
    CHECK (fx.activeVoiceCount() == 1);
    CHECK (fx.playingNote() == 60);
    CHECK_THAT (fx.trackedFrequency(), WithinRel (noteHz (64), 1e-4f));

    // Releasing the top note glides back down to the still-held note.
    synth.noteOff (1, 64, 0.0f, true);
    CHECK_THAT (fx.trackedFrequency(), WithinRel (noteHz (60), 1e-4f));

    synth.noteOff (1, 60, 0.0f, true);
    fx.renderSeconds (1.0);
    CHECK (fx.activeVoiceCount() == 0);
}

TEST_CASE ("Sustain pedal defers note-off in mono mode", "[voices]")
{
    SynthFixture fx;
    fx.enableMono();
    auto& synth = fx.plugin.getSynth();

    synth.handleSustainPedal (1, true);
    synth.noteOn (1, 60, 0.8f);
    synth.noteOff (1, 60, 0.0f, true);

    // Pedal is down, so the note keeps sounding
    CHECK (fx.activeVoiceCount() == 1);
    CHECK (fx.playingNote() == 60);

    // Releasing the pedal finally processes the deferred note-off
    synth.handleSustainPedal (1, false);
    fx.renderSeconds (1.0);
    CHECK (fx.activeVoiceCount() == 0);
}

TEST_CASE ("Sustain pedal holds released notes in poly mode", "[voices]")
{
    SynthFixture fx;
    auto& synth = fx.plugin.getSynth();

    synth.handleSustainPedal (1, true);
    synth.noteOn (1, 60, 0.8f);
    synth.noteOn (1, 64, 0.8f);
    synth.noteOff (1, 60, 0.0f, true);
    synth.noteOff (1, 64, 0.0f, true);

    CHECK (fx.activeVoiceCount() == 2);

    synth.handleSustainPedal (1, false);
    fx.renderSeconds (1.0);
    CHECK (fx.activeVoiceCount() == 0);
}

TEST_CASE ("allNotesOff silences everything immediately", "[voices]")
{
    SynthFixture fx;
    auto& synth = fx.plugin.getSynth();

    synth.noteOn (1, 60, 0.8f);
    synth.noteOn (1, 64, 0.8f);
    synth.handleSustainPedal (1, true);
    REQUIRE (fx.activeVoiceCount() == 2);

    synth.allNotesOff (0, false);
    CHECK (fx.activeVoiceCount() == 0);
}
