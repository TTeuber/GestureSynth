#pragma once

#include <array>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>

namespace UpdateCheck
{
    // Where releases live. The API endpoint returns JSON with tag_name/html_url;
    // the releases page is the fallback link shown to the user.
    static constexpr auto latestReleaseEndpoint = "https://api.github.com/repos/TTeuber/GestureSynth/releases/latest";
    static constexpr auto releasesPageURL = "https://github.com/TTeuber/GestureSynth/releases";

    // Compares two dotted version strings numerically (tolerates a leading "v"
    // and trailing pre-release suffixes like "-beta"). Pure so it's testable.
    inline bool isNewerVersion (const juce::String& currentVersion, const juce::String& latestVersion)
    {
        auto parse = [] (const juce::String& v)
        {
            juce::StringArray parts;
            parts.addTokens (v.trim().trimCharactersAtStart ("vV"), ".", "");
            std::array<int, 3> numbers { 0, 0, 0 };
            for (size_t i = 0; i < numbers.size() && (int) i < parts.size(); ++i)
                numbers[i] = parts[(int) i].getIntValue(); // stops at first non-digit, so "3-beta" -> 3
            return numbers;
        };

        if (latestVersion.trim().trimCharactersAtStart ("vV").isEmpty())
            return false;

        return parse (latestVersion) > parse (currentVersion);
    }
}

//==============================================================================
// Checks GitHub for a newer release on a background thread, at most once per
// day (the result is cached in a properties file between checks). Failures are
// silent: no network, rate-limited, sandboxed host — all just mean "no update".
class UpdateChecker : private juce::Thread
{
public:
    explicit UpdateChecker (const juce::String& currentVersionToUse)
        : juce::Thread ("UpdateChecker"), currentVersion (currentVersionToUse) {}

    ~UpdateChecker() override
    {
        stopThread (6000);
    }

    // Called on the message thread when a newer version is known.
    std::function<void (const juce::String& newVersion, const juce::String& releaseURL)> onUpdateAvailable;

    void checkNow()
    {
        startThread();
    }

private:
    void run() override
    {
        auto props = createPropertiesFile();
        if (props == nullptr)
            return;

        const auto now = juce::Time::currentTimeMillis();
        const auto lastCheck = props->getValue ("lastCheckTime", "0").getLargeIntValue();
        auto latestVersion = props->getValue ("latestKnownVersion");
        auto releaseURL = props->getValue ("latestReleaseURL", UpdateCheck::releasesPageURL);

        constexpr juce::int64 dayMs = 24 * 60 * 60 * 1000;

        if (now - lastCheck >= dayMs || now < lastCheck)
        {
            const auto fetched = fetchLatestVersion();

            if (threadShouldExit())
                return;

            if (fetched.gotResponse)
            {
                props->setValue ("lastCheckTime", juce::String (now));

                if (fetched.version.isNotEmpty())
                {
                    latestVersion = fetched.version;
                    releaseURL = fetched.url;
                    props->setValue ("latestKnownVersion", latestVersion);
                    props->setValue ("latestReleaseURL", releaseURL);
                }

                props->saveIfNeeded();
            }
        }

        if (UpdateCheck::isNewerVersion (currentVersion, latestVersion))
        {
            juce::MessageManager::callAsync (
                [weakThis = juce::WeakReference<UpdateChecker> (this), latestVersion, releaseURL]
                {
                    if (weakThis != nullptr && weakThis->onUpdateAvailable != nullptr)
                        weakThis->onUpdateAvailable (latestVersion.trimCharactersAtStart ("vV"), releaseURL);
                });
        }
    }

    struct FetchResult
    {
        bool gotResponse = false;
        juce::String version, url;
    };

    FetchResult fetchLatestVersion()
    {
        const auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                                 .withConnectionTimeoutMs (5000)
                                 .withExtraHeaders ("Accept: application/vnd.github+json\r\n"
                                                    "User-Agent: GestureSynth-UpdateChecker")
                                 .withProgressCallback ([this] (int, int) { return ! threadShouldExit(); });

        auto stream = juce::URL (UpdateCheck::latestReleaseEndpoint).createInputStream (options);
        if (stream == nullptr)
            return {};

        const auto response = juce::JSON::parse (stream->readEntireStreamAsString());

        FetchResult result;
        result.gotResponse = true;
        result.version = response.getProperty ("tag_name", {}).toString();
        result.url = response.getProperty ("html_url", UpdateCheck::releasesPageURL).toString();
        return result;
    }

    std::unique_ptr<juce::PropertiesFile> createPropertiesFile()
    {
        juce::PropertiesFile::Options options;
        options.applicationName = "GestureSynthUpdateCheck";
        options.filenameSuffix = ".settings";
        options.folderName = PRODUCT_NAME_WITHOUT_VERSION;
        options.osxLibrarySubFolder = "Application Support";
        return std::make_unique<juce::PropertiesFile> (options);
    }

    const juce::String currentVersion;

    JUCE_DECLARE_WEAK_REFERENCEABLE (UpdateChecker)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UpdateChecker)
};
