#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <map>
#include <vector>

class PresetManager
{
public:
    static constexpr int kPresetFormatVersion = 1;

    struct PresetInfo
    {
        juce::String name;
        juce::File file;
    };

    PresetManager() = default;

    // Redirects all preset IO to the given directory (used by unit tests so
    // they never touch the user's real preset folder).
    void setPresetsDirectoryOverride (const juce::File& dir) { presetsDirOverride = dir; }

    juce::File getPresetsDirectory() const;

    std::map<juce::String, std::vector<PresetInfo>> scanPresets() const;

    bool savePreset (const juce::String& name,
                     const juce::String& category,
                     const juce::ValueTree& stateTree) const;

    juce::File resolvePresetFile (const juce::String& name, const juce::String& category) const;
    bool presetExists (const juce::String& name, const juce::String& category) const;

    juce::ValueTree loadPreset (const juce::File& presetFile) const;

    // Extracts the factory presets embedded in the plugin binary (assets/presets)
    // into the user presets folder. Existing files are never overwritten, and a
    // version stamp file makes repeat launches a no-op — so user edits and
    // deletions are respected until the stamp changes. Returns the number of
    // preset files written.
    int installFactoryPresets (const juce::String& versionStamp) const;

    juce::PopupMenu buildMenu (std::map<int, juce::File>& idToFileMap) const;

    juce::StringArray getCategories() const;

    bool deletePreset (const juce::File& presetFile) const;

    std::vector<PresetInfo> getFlatPresetList() const;

private:
    juce::File presetsDirOverride;
};
