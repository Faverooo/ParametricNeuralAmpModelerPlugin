#include "LevelMatch.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace
{
void check(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}
}

int main()
{
    try
    {
        LevelMatch match;
        match.setEnabled(true);
        match.measure(0, 100, 48000, 1, 48000);
        check(match.measuring && match.activeSeconds == 0, "Silent input advanced the measurement");

        for (int second = 0; second < 5; ++second)
            match.measure(4000, 1000, 48000, 1, 48000);
        check(!match.measuring && std::abs(match.targetDb() - 6.0206f) < 0.001f, "Incorrect learned gain");

        match.setEnabled(false);
        check(match.targetDb() == 0 && match.targetDb(-4.0f) == -4.0f, "Manual output trim not applied");
        match.setEnabled(true);
        check(!match.measuring && match.targetDb(-4.0f) > 6.0f, "Enabling lost the learned gain");

        match.start();
        match.measure(1000, 4000, 240000, 1, 48000);
        check(std::abs(match.targetDb() + 6.0206f) < 0.001f, "Relearn did not replace the gain");

        match.start();
        match.measure(1000, 0, 240000, 1, 48000);
        check(match.noOutput && !match.measuring, "Silent output accepted for matching");

        match.start();
        match.measure(1000, 0.001, 240000, 1, 48000);
        check(match.targetDb() == LevelMatch::maximumCorrectionDb, "Gain limit ignored");
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
