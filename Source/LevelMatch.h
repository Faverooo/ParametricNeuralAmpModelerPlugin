#pragma once

#include <algorithm>
#include <cmath>
#include <optional>

class LevelMatch
{
public:
    static constexpr double durationSeconds = 5.0;
    static constexpr float maximumCorrectionDb = 18.0f;

    void setEnabled(bool shouldEnable)
    {
        enabled = shouldEnable;
        if (!enabled)
            measuring = false;
        else if (!correctionDb)
            start();
    }

    void start()
    {
        enabled = true;
        measuring = true;
        noOutput = false;
        activeSeconds = dryEnergy = wetEnergy = 0.0;
    }

    void measure(double dry, double wet, int frames, int channels, double sampleRate)
    {
        // A block is active above -60 dBFS RMS on the dry signal.
        if (dry < 1.0e-6 * frames * channels || !std::isfinite(dry) || !std::isfinite(wet))
            return;

        const double seconds = double(frames) / sampleRate;
        const double fraction = std::clamp((durationSeconds - activeSeconds) / seconds, 0.0, 1.0);
        dryEnergy += dry * fraction;
        wetEnergy += wet * fraction;
        activeSeconds += seconds;
        if (activeSeconds < durationSeconds)
            return;

        measuring = false;
        // A silent output has no meaningful gain match.
        noOutput = wetEnergy <= dryEnergy * 1.0e-12;
        if (!noOutput)
        {
            const auto db = static_cast<float>(10.0 * std::log10(dryEnergy / wetEnergy));
            correctionDb = std::clamp(db, -maximumCorrectionDb, maximumCorrectionDb);
        }
    }

    float targetDb() const { return enabled ? correctionDb.value_or(0.0f) : 0.0f; }

    bool enabled = false;
    bool measuring = false;
    bool noOutput = false;
    double activeSeconds = 0.0;
    std::optional<float> correctionDb;

private:
    double dryEnergy = 0.0;
    double wetEnergy = 0.0;
};
