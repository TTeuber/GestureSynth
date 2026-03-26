#include "PresetManager.h"

juce::File PresetManager::getPresetsDirectory() const
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("SynthDemo")
                   .getChildFile ("Presets");

    if (! dir.exists())
        dir.createDirectory();

    return dir;
}

std::map<juce::String, std::vector<PresetManager::PresetInfo>> PresetManager::scanPresets() const
{
    std::map<juce::String, std::vector<PresetInfo>> result;
    auto presetsDir = getPresetsDirectory();

    // Scan root directory for uncategorized presets
    for (const auto& file : presetsDir.findChildFiles (juce::File::findFiles, false, "*.xml"))
        result["Uncategorized"].push_back ({ file.getFileNameWithoutExtension(), file });

    // Scan subdirectories as categories
    for (const auto& subDir : presetsDir.findChildFiles (juce::File::findDirectories, false))
    {
        auto category = subDir.getFileName();
        for (const auto& file : subDir.findChildFiles (juce::File::findFiles, false, "*.xml"))
            result[category].push_back ({ file.getFileNameWithoutExtension(), file });
    }

    return result;
}

bool PresetManager::savePreset (const juce::String& name,
                                const juce::String& category,
                                const juce::ValueTree& stateTree) const
{
    juce::ValueTree preset ("SynthDemoPreset");
    preset.setProperty ("name", name, nullptr);
    preset.setProperty ("category", category.isEmpty() ? "Uncategorized" : category, nullptr);
    preset.setProperty ("formatVersion", kPresetFormatVersion, nullptr);
    preset.setProperty ("pluginVersion", juce::String (VERSION), nullptr);

    preset.addChild (stateTree.createCopy(), -1, nullptr);

    auto xml = preset.createXml();
    if (xml == nullptr)
        return false;

    auto presetsDir = getPresetsDirectory();
    juce::File outputDir;

    if (category.isEmpty() || category == "Uncategorized")
        outputDir = presetsDir;
    else
    {
        outputDir = presetsDir.getChildFile (category);
        if (! outputDir.exists())
            outputDir.createDirectory();
    }

    auto outputFile = outputDir.getChildFile (name + ".xml");
    return xml->writeTo (outputFile);
}

juce::ValueTree PresetManager::loadPreset (const juce::File& presetFile) const
{
    auto xml = juce::XmlDocument::parse (presetFile);
    if (xml == nullptr)
        return {};

    auto preset = juce::ValueTree::fromXml (*xml);
    if (! preset.isValid() || preset.getType().toString() != "SynthDemoPreset")
        return {};

    // Return the inner PluginState child
    auto pluginState = preset.getChildWithName ("PluginState");
    if (pluginState.isValid())
        return pluginState;

    // Fallback: return first child if it exists
    if (preset.getNumChildren() > 0)
        return preset.getChild (0);

    return {};
}

juce::PopupMenu PresetManager::buildMenu (std::map<int, juce::File>& idToFileMap) const
{
    juce::PopupMenu menu;
    auto presets = scanPresets();
    int nextId = 1;

    for (auto& [category, presetList] : presets)
    {
        if (presetList.empty())
            continue;

        // Sort presets alphabetically within each category
        std::sort (presetList.begin(), presetList.end(),
                   [] (const PresetInfo& a, const PresetInfo& b) { return a.name < b.name; });

        juce::PopupMenu subMenu;
        for (const auto& preset : presetList)
        {
            idToFileMap[nextId] = preset.file;
            subMenu.addItem (nextId, preset.name);
            ++nextId;
        }

        menu.addSubMenu (category, subMenu);
    }

    if (menu.getNumItems() == 0)
        menu.addItem (-1, "(No presets found)", false, false);

    return menu;
}

juce::StringArray PresetManager::getCategories() const
{
    juce::StringArray categories;
    auto presetsDir = getPresetsDirectory();

    for (const auto& subDir : presetsDir.findChildFiles (juce::File::findDirectories, false))
        categories.add (subDir.getFileName());

    categories.sort (false);
    return categories;
}

bool PresetManager::deletePreset (const juce::File& presetFile) const
{
    return presetFile.deleteFile();
}
