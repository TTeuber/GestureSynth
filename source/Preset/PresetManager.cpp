#include "PresetManager.h"
#include "BinaryData.h"

juce::File PresetManager::getPresetsDirectory() const
{
    if (presetsDirOverride != juce::File())
    {
        if (! presetsDirOverride.exists())
            presetsDirOverride.createDirectory();
        return presetsDirOverride;
    }

    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("GestureSynth")
                   .getChildFile ("Presets");

    if (! dir.exists())
    {
        // One-time migration of presets saved under the pre-rename app name
        auto legacyDir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                             .getChildFile ("SynthDemo")
                             .getChildFile ("Presets");

        if (legacyDir.isDirectory())
            legacyDir.copyDirectoryTo (dir);
        else
            dir.createDirectory();
    }

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

juce::File PresetManager::resolvePresetFile (const juce::String& name, const juce::String& category) const
{
    auto presetsDir = getPresetsDirectory();

    if (category.isEmpty() || category == "Uncategorized")
        return presetsDir.getChildFile (name + ".xml");

    return presetsDir.getChildFile (category).getChildFile (name + ".xml");
}

bool PresetManager::presetExists (const juce::String& name, const juce::String& category) const
{
    return resolvePresetFile (name, category).existsAsFile();
}

bool PresetManager::savePreset (const juce::String& name,
                                const juce::String& category,
                                const juce::ValueTree& stateTree) const
{
    juce::ValueTree preset ("GestureSynthPreset");
    preset.setProperty ("name", name, nullptr);
    preset.setProperty ("category", category.isEmpty() ? "Uncategorized" : category, nullptr);
    preset.setProperty ("formatVersion", kPresetFormatVersion, nullptr);
    preset.setProperty ("pluginVersion", juce::String (VERSION), nullptr);

    preset.addChild (stateTree.createCopy(), -1, nullptr);

    auto xml = preset.createXml();
    if (xml == nullptr)
        return false;

    auto outputFile = resolvePresetFile (name, category);
    auto outputDir = outputFile.getParentDirectory();
    if (! outputDir.exists())
        outputDir.createDirectory();

    return xml->writeTo (outputFile);
}

juce::ValueTree PresetManager::loadPreset (const juce::File& presetFile) const
{
    auto xml = juce::XmlDocument::parse (presetFile);
    if (xml == nullptr)
        return {};

    auto preset = juce::ValueTree::fromXml (*xml);
    auto type = preset.getType().toString();

    // "SynthDemoPreset" is the legacy tag from before the GestureSynth rename
    if (! preset.isValid() || (type != "GestureSynthPreset" && type != "SynthDemoPreset"))
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

int PresetManager::installFactoryPresets (const juce::String& versionStamp) const
{
    auto presetsDir = getPresetsDirectory();
    auto stampFile = presetsDir.getChildFile (".factory-presets-version");

    if (stampFile.existsAsFile() && stampFile.loadFileAsString().trim() == versionStamp)
        return 0;

    int installed = 0;

    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        const char* resourceName = BinaryData::namedResourceList[i];
        const char* originalName = BinaryData::getNamedResourceOriginalFilename (resourceName);
        if (originalName == nullptr || ! juce::String (originalName).endsWithIgnoreCase (".xml"))
            continue;

        int dataSize = 0;
        const char* data = BinaryData::getNamedResource (resourceName, dataSize);
        if (data == nullptr || dataSize <= 0)
            continue;

        auto xml = juce::XmlDocument::parse (juce::String::fromUTF8 (data, dataSize));
        if (xml == nullptr || xml->getTagName() != "GestureSynthPreset")
            continue;

        auto name = xml->getStringAttribute ("name",
            juce::String (originalName).upToLastOccurrenceOf (".", false, false));
        auto category = xml->getStringAttribute ("category", "Uncategorized");

        auto target = resolvePresetFile (name, category);
        if (target.existsAsFile())
            continue; // never clobber a user-edited preset of the same name

        auto targetDir = target.getParentDirectory();
        if (! targetDir.exists())
            targetDir.createDirectory();

        if (xml->writeTo (target))
            ++installed;
    }

    stampFile.replaceWithText (versionStamp + "\n");
    return installed;
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

std::vector<PresetManager::PresetInfo> PresetManager::getFlatPresetList() const
{
    std::vector<PresetInfo> result;
    auto presets = scanPresets();

    for (auto& [category, presetList] : presets)
    {
        std::sort (presetList.begin(), presetList.end(),
                   [] (const PresetInfo& a, const PresetInfo& b) { return a.name < b.name; });

        for (const auto& preset : presetList)
            result.push_back (preset);
    }

    return result;
}
