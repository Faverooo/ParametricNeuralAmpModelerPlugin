#pragma once

#include <array>
#include <juce_audio_processors/juce_audio_processors.h>
#include "NAM/pnam.h"
#include "LevelMatch.h"

class ParametricNAMProcessor final : public juce::AudioProcessor
{
public:
    static constexpr size_t maximumControls = 8;

    struct ModelInfo
    {
        juce::String amplifier;
        std::vector<nam::ParametricControl> controls;
        double sampleRate = 0;
        juce::String error;
        bool modelReady = false;
        LevelMatch match;
    };

    ParametricNAMProcessor();

    void prepareToPlay(double sampleRate, int maximumBlockSize) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "ParametricNAM"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    void openModel(const juce::File&);
    ModelInfo modelInfo();
    void enableLevelMatch(bool enabled);
    void relearnLevelMatch();
    static juce::String controlId(size_t index);
    juce::AudioProcessorValueTreeState parameters;

private:
    struct Model
    {
        explicit Model(const std::filesystem::path& path);
        std::array<nam::ParametricModel, 2> channels;
        std::array<float, maximumControls> lastValues{};
        bool controlsSet = false;
        bool prepare(double rate, int maximumBlockSize);
    };

    void loadModel(const juce::File&, bool setDefaults);
    std::array<std::atomic<float>*, maximumControls> controlValues{};
    std::unique_ptr<Model> model;
    juce::File modelFile;
    juce::String modelError;
    double processingRate = 0;
    int blockCapacity = 0;
    bool modelReady = false;
    LevelMatch levelMatch;
    juce::SmoothedValue<float> matchGain{1.0f};
};
