#pragma once

#include <juce_core/juce_core.h>

#include <array>
#include <charconv>
#include <mutex>
#include <optional>

class UpdateCheck final : private juce::Thread
{
public:
    UpdateCheck(const juce::String& repository, const juce::String& installedVersion)
        : Thread("Check GitHub release"),
          current(parseVersion(installedVersion)),
          releasePage("https://github.com/" + repository + "/releases/latest"),
          stream(juce::URL("https://api.github.com/repos/" + repository + "/releases/latest"), false)
    {
        stream.withConnectionTimeout(3000).withNumRedirectsToFollow(0).withExtraHeaders(
            "Accept: application/vnd.github+json\r\nUser-Agent: ParametricNAM-UpdateCheck\r\n");
        if (current)
            startThread();
    }

    ~UpdateCheck() override
    {
        signalThreadShouldExit();
        stream.cancel();
        stopThread(-1);
    }

    juce::String availableVersion() const
    {
        const std::scoped_lock lock(resultMutex);
        return available;
    }

    void openReleasePage() const { juce::URL(releasePage).launchInDefaultBrowser(); }

    static std::optional<std::array<int, 3>> parseVersion(juce::String version)
    {
        if (version.startsWithChar('v'))
            version = version.substring(1);
        const auto parts = juce::StringArray::fromTokens(version, ".", "");
        if (parts.size() != 3)
            return std::nullopt;
        std::array<int, 3> result{};
        for (size_t i = 0; i < result.size(); ++i)
        {
            const auto part = parts[static_cast<int>(i)];
            if (part.isEmpty() || !part.containsOnly("0123456789"))
                return std::nullopt;
            const auto digits = part.toStdString();
            const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), result[i]);
            if (error != std::errc{} || end != digits.data() + digits.size())
                return std::nullopt;
        }
        return result;
    }

private:
    void run() override
    {
        if (!stream.connect(nullptr) || stream.getStatusCode() != 200 || threadShouldExit())
            return;
        juce::MemoryBlock body;
        stream.readIntoMemoryBlock(body, 128 * 1024);
        if (threadShouldExit())
            return;
        const auto release = juce::JSON::parse(body.toString());
        const auto tag = release.getProperty("tag_name", {}).toString();
        const auto remote = parseVersion(tag);
        if (remote && current && *remote > *current)
        {
            const std::scoped_lock lock(resultMutex);
            available = tag;
        }
    }

    const std::optional<std::array<int, 3>> current;
    const juce::String releasePage;
    juce::WebInputStream stream;
    mutable std::mutex resultMutex;
    juce::String available;
};
