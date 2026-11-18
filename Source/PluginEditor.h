#pragma once

#include "PluginProcessor.h"

class ParametricNAMEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ParametricNAMEditor(ParametricNAMProcessor&);
    ~ParametricNAMEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct Theme final : juce::LookAndFeel_V4
    {
        void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
        void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
        void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    };

    void timerCallback() override;
    void chooseModel();

    ParametricNAMProcessor& processor;
    ParametricNAMProcessor::ModelInfo info;
    Theme theme;
    juce::TextButton openButton{"Load model"};
    juce::Label identity{"identity", "PARAMETRICNAM"};
    juce::Label amplifier;
    juce::Label status;
    juce::ToggleButton matchButton{"Level Match"};
    juce::TextButton relearnButton{"Relearn"};
    juce::Label matchStatus;
    std::array<juce::Rectangle<int>, ParametricNAMProcessor::maximumControls> cards;
    std::array<juce::Label, ParametricNAMProcessor::maximumControls> labels;
    std::array<juce::Slider, ParametricNAMProcessor::maximumControls> dials;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,
               ParametricNAMProcessor::maximumControls> attachments;
    std::unique_ptr<juce::FileChooser> chooser;
};
