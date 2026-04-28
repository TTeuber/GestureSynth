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

    juce::File getPresetsDirectory() const;

    std::map<juce::String, std::vector<PresetInfo>> scanPresets() const;

    bool savePreset (const juce::String& name,
                     const juce::String& category,
                     const juce::ValueTree& stateTree) const;

    juce::File resolvePresetFile (const juce::String& name, const juce::String& category) const;
    bool presetExists (const juce::String& name, const juce::String& category) const;

    juce::ValueTree loadPreset (const juce::File& presetFile) const;

    juce::PopupMenu buildMenu (std::map<int, juce::File>& idToFileMap) const;

    juce::StringArray getCategories() const;

    bool deletePreset (const juce::File& presetFile) const;

    std::vector<PresetInfo> getFlatPresetList() const;
};
