#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

// End-to-end coverage of the gesture-drag modulation pipeline: editing the
// modTree (as the UI does) must reach the voices' mod matrices via their
// ValueTree listeners and the lock-free command queue, and show up in the
// modDestOutputs the editor reads back.
namespace
{
struct RoutingFixture
{
    RoutingFixture()
    {
        plugin.prepareToPlay (44100.0, 512);

        // Mod wheel is a constant-value source, so the expected destination
        // offset is exact. Pin it at full.
        plugin.getSynth().handleController (1, 1, 127);
    }

    void render (int numBlocks = 4)
    {
        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer midi;
        for (int i = 0; i < numBlocks; ++i)
        {
            buffer.clear();
            plugin.getSynth().renderNextBlock (buffer, midi, 0, 512);
        }
    }

    float filterCutoffOutput() const
    {
        return plugin.modDestOutputs[static_cast<size_t> (ModDest::filterCutoff)].load();
    }

    PluginProcessor plugin;
};
} // namespace

TEST_CASE ("modTree edits route modulation into the rendered voice", "[modrouting]")
{
    RoutingFixture fx;

    // Route mod wheel -> filter cutoff at -0.5 depth, as a gesture drag would
    auto slot = fx.plugin.modTree.getChild (0);
    slot.setProperty ("source", "modWheel", nullptr);
    slot.setProperty ("destination", "filterFrequency", nullptr);
    slot.setProperty ("depth", -0.5f, nullptr);

    fx.plugin.getSynth().noteOn (1, 60, 0.8f);
    fx.render();

    // Cutoff defaults to normalized 1.0; wheel at 1.0 * depth -0.5 => 0.5
    CHECK_THAT (fx.filterCutoffOutput(), WithinAbs (0.5f, 1e-4f));

    SECTION ("changing the depth updates the routing in place")
    {
        slot.setProperty ("depth", -0.25f, nullptr);
        fx.render();
        CHECK_THAT (fx.filterCutoffOutput(), WithinAbs (0.75f, 1e-4f));
    }

    SECTION ("bypassing a slot suspends its modulation, un-bypassing restores it")
    {
        slot.setProperty ("bypassed", true, nullptr);
        fx.render();
        CHECK_THAT (fx.filterCutoffOutput(), WithinAbs (1.0f, 1e-4f));

        slot.setProperty ("bypassed", false, nullptr);
        fx.render();
        CHECK_THAT (fx.filterCutoffOutput(), WithinAbs (0.5f, 1e-4f));
    }

    SECTION ("clearing the slot removes the modulation")
    {
        slot.setProperty ("source", "None", nullptr);
        slot.setProperty ("destination", "None", nullptr);
        fx.render();
        CHECK_THAT (fx.filterCutoffOutput(), WithinAbs (1.0f, 1e-4f));
    }
}
