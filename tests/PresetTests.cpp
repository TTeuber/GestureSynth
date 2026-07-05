#include <PluginProcessor.h>
#include <Preset/PresetManager.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

namespace
{
// RAII temp preset directory so tests never touch the user's real presets.
struct TempPresetDir
{
    TempPresetDir()
    {
        dir = juce::File::getSpecialLocation (juce::File::tempDirectory)
                  .getChildFile ("GestureSynthTests")
                  .getNonexistentChildFile ("presets", "");
        dir.createDirectory();
    }
    ~TempPresetDir() { dir.getParentDirectory().deleteRecursively(); }
    juce::File dir;
};

void setParam (PluginProcessor& plugin, const char* id, float normalizedValue)
{
    plugin.parameters.getParameter (id)->setValueNotifyingHost (normalizedValue);
}

float getParam (PluginProcessor& plugin, const char* id)
{
    return plugin.parameters.getParameter (id)->getValue();
}
} // namespace

TEST_CASE ("Full plugin state survives a save/restore round-trip", "[preset]")
{
    PluginProcessor saved;

    // Dial in a distinctly non-default patch...
    setParam (saved, ParamIDs::volume, 0.85f);
    setParam (saved, ParamIDs::filterFrequency, 0.33f);
    setParam (saved, ParamIDs::monoOn, 1.0f);

    // ...including a gesture-drag modulation routing in slot 0
    auto slot = saved.modTree.getChild (0);
    slot.setProperty ("source", "lfo1", nullptr);
    slot.setProperty ("destination", "filterFrequency", nullptr);
    slot.setProperty ("depth", 0.42f, nullptr);
    slot.setProperty ("isBipolar", true, nullptr);

    juce::MemoryBlock stateData;
    saved.getStateInformation (stateData);

    PluginProcessor restored;
    restored.setStateInformation (stateData.getData(), static_cast<int> (stateData.getSize()));

    CHECK_THAT (getParam (restored, ParamIDs::volume), WithinAbs (0.85f, 1e-4f));
    CHECK_THAT (getParam (restored, ParamIDs::filterFrequency), WithinAbs (0.33f, 1e-4f));
    CHECK_THAT (getParam (restored, ParamIDs::monoOn), WithinAbs (1.0f, 1e-4f));

    auto restoredSlot = restored.modTree.getChild (0);
    CHECK (restoredSlot.getProperty ("source").toString() == "lfo1");
    CHECK (restoredSlot.getProperty ("destination").toString() == "filterFrequency");
    CHECK_THAT (static_cast<float> (restoredSlot.getProperty ("depth")), WithinAbs (0.42f, 1e-6f));
    CHECK (static_cast<bool> (restoredSlot.getProperty ("isBipolar")));
}

TEST_CASE ("PresetManager round-trips a preset through disk", "[preset]")
{
    TempPresetDir tempDir;
    PluginProcessor plugin;
    plugin.presetManager.setPresetsDirectoryOverride (tempDir.dir);

    setParam (plugin, ParamIDs::volume, 0.77f);
    auto slot = plugin.modTree.getChild (2);
    slot.setProperty ("source", "env2", nullptr);
    slot.setProperty ("destination", "pulseWidth", nullptr);
    slot.setProperty ("depth", -0.25f, nullptr);

    const auto stateTree = plugin.buildStateTree();
    REQUIRE (plugin.presetManager.savePreset ("Test Patch", "Bass", stateTree));
    REQUIRE (plugin.presetManager.presetExists ("Test Patch", "Bass"));

    const auto presetFile = plugin.presetManager.resolvePresetFile ("Test Patch", "Bass");
    REQUIRE (presetFile.existsAsFile());

    const auto loaded = plugin.presetManager.loadPreset (presetFile);
    REQUIRE (loaded.isValid());
    CHECK (loaded.getType().toString() == "PluginState");
    CHECK (loaded.isEquivalentTo (stateTree));

    // Applying the loaded preset to a fresh processor restores the patch
    PluginProcessor restored;
    restored.restoreFromStateTree (loaded);
    CHECK_THAT (getParam (restored, ParamIDs::volume), WithinAbs (0.77f, 1e-4f));
    auto restoredSlot = restored.modTree.getChild (2);
    CHECK (restoredSlot.getProperty ("source").toString() == "env2");
    CHECK (restoredSlot.getProperty ("destination").toString() == "pulseWidth");
    CHECK_THAT (static_cast<float> (restoredSlot.getProperty ("depth")), WithinAbs (-0.25f, 1e-6f));
}

TEST_CASE ("Preset files carry name, category and format version metadata", "[preset]")
{
    TempPresetDir tempDir;
    PluginProcessor plugin;
    plugin.presetManager.setPresetsDirectoryOverride (tempDir.dir);

    REQUIRE (plugin.presetManager.savePreset ("Meta Check", "Leads", plugin.buildStateTree()));

    const auto xml = juce::XmlDocument::parse (
        plugin.presetManager.resolvePresetFile ("Meta Check", "Leads"));
    REQUIRE (xml != nullptr);

    CHECK (xml->getTagName() == "GestureSynthPreset");
    CHECK (xml->getStringAttribute ("name") == "Meta Check");
    CHECK (xml->getStringAttribute ("category") == "Leads");
    CHECK (xml->getIntAttribute ("formatVersion") == PresetManager::kPresetFormatVersion);
}

TEST_CASE ("Presets with the legacy SynthDemoPreset tag still load", "[preset]")
{
    TempPresetDir tempDir;
    PluginProcessor plugin;
    plugin.presetManager.setPresetsDirectoryOverride (tempDir.dir);

    // Recreate a preset saved before the GestureSynth rename
    juce::ValueTree legacy ("SynthDemoPreset");
    legacy.setProperty ("name", "Old Patch", nullptr);
    legacy.addChild (plugin.buildStateTree(), -1, nullptr);

    const auto legacyFile = tempDir.dir.getChildFile ("Old Patch.xml");
    REQUIRE (legacy.createXml()->writeTo (legacyFile));

    const auto loaded = plugin.presetManager.loadPreset (legacyFile);
    REQUIRE (loaded.isValid());
    CHECK (loaded.getType().toString() == "PluginState");
}

TEST_CASE ("Unrecognized or corrupt preset files are rejected", "[preset]")
{
    TempPresetDir tempDir;
    PresetManager manager;
    manager.setPresetsDirectoryOverride (tempDir.dir);

    SECTION ("wrong root tag")
    {
        juce::ValueTree bogus ("SomeOtherPlugin");
        const auto file = tempDir.dir.getChildFile ("bogus.xml");
        REQUIRE (bogus.createXml()->writeTo (file));
        CHECK_FALSE (manager.loadPreset (file).isValid());
    }

    SECTION ("not XML at all")
    {
        const auto file = tempDir.dir.getChildFile ("garbage.xml");
        file.replaceWithText ("this is not xml");
        CHECK_FALSE (manager.loadPreset (file).isValid());
    }

    SECTION ("missing file")
    {
        CHECK_FALSE (manager.loadPreset (tempDir.dir.getChildFile ("nope.xml")).isValid());
    }
}

TEST_CASE ("Factory presets install from the binary into the preset folder", "[preset]")
{
    TempPresetDir tempDir;
    PresetManager manager;
    manager.setPresetsDirectoryOverride (tempDir.dir);

    const int installed = manager.installFactoryPresets ("test-stamp-1");
    REQUIRE (installed > 0);

    // Sound-design brief: at least these categories must exist
    auto categories = manager.getCategories();
    for (auto expected : { "Bass", "Leads", "Pads", "Keys", "Plucks", "FX", "Templates" })
    {
        INFO ("missing category: " << expected);
        CHECK (categories.contains (expected));
    }

    CHECK (static_cast<int> (manager.getFlatPresetList().size()) == installed);

    SECTION ("second call with the same stamp is a no-op")
    {
        CHECK (manager.installFactoryPresets ("test-stamp-1") == 0);
    }

    SECTION ("user deletions are respected until the stamp changes")
    {
        const auto victim = manager.getFlatPresetList().front().file;
        REQUIRE (victim.deleteFile());

        CHECK (manager.installFactoryPresets ("test-stamp-1") == 0);
        CHECK_FALSE (victim.existsAsFile());

        // A version bump restores only the missing file
        CHECK (manager.installFactoryPresets ("test-stamp-2") == 1);
        CHECK (victim.existsAsFile());
    }

    SECTION ("user-edited presets are never overwritten")
    {
        const auto edited = manager.getFlatPresetList().front().file;
        REQUIRE (edited.replaceWithText ("<GestureSynthPreset name=\"user edit\"/>"));

        manager.installFactoryPresets ("test-stamp-3");
        CHECK (edited.loadFileAsString().contains ("user edit"));
    }
}

TEST_CASE ("Every factory preset loads cleanly into the processor", "[preset]")
{
    TempPresetDir tempDir;
    PluginProcessor plugin;
    plugin.presetManager.setPresetsDirectoryOverride (tempDir.dir);
    REQUIRE (plugin.presetManager.installFactoryPresets ("load-check") > 0);

    for (const auto& info : plugin.presetManager.getFlatPresetList())
    {
        INFO ("preset: " << info.name);

        const auto state = plugin.presetManager.loadPreset (info.file);
        REQUIRE (state.isValid());
        REQUIRE (state.getType().toString() == "PluginState");

        // Factory presets pin every parameter explicitly so loads are deterministic
        auto paramState = state.getChildWithName (plugin.parameters.state.getType());
        REQUIRE (paramState.isValid());
        for (const auto* param : plugin.getParameters())
            if (const auto* withID = dynamic_cast<const juce::AudioProcessorParameterWithID*> (param))
            {
                INFO ("parameter not pinned by preset: " << withID->paramID);
                CHECK (paramState.getChildWithProperty ("id", withID->paramID).isValid());
            }

        plugin.restoreFromStateTree (state);

        // The mod matrix keeps its full complement of slots after the load
        CHECK (plugin.modTree.getNumChildren() >= 12);
    }
}

TEST_CASE ("scanPresets finds categorized and uncategorized presets", "[preset]")
{
    TempPresetDir tempDir;
    PluginProcessor plugin;
    plugin.presetManager.setPresetsDirectoryOverride (tempDir.dir);

    const auto state = plugin.buildStateTree();
    REQUIRE (plugin.presetManager.savePreset ("Rooty", "", state));
    REQUIRE (plugin.presetManager.savePreset ("Bassy", "Bass", state));
    REQUIRE (plugin.presetManager.savePreset ("Growly", "Bass", state));

    auto scanned = plugin.presetManager.scanPresets();
    REQUIRE (scanned.count ("Uncategorized") == 1);
    REQUIRE (scanned.count ("Bass") == 1);
    CHECK (scanned["Uncategorized"].size() == 1);
    CHECK (scanned["Bass"].size() == 2);

    CHECK (plugin.presetManager.getCategories() == juce::StringArray { "Bass" });
    CHECK (plugin.presetManager.getFlatPresetList().size() == 3);
}
