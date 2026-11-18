#pragma once

#include "PluginProcessor.h"
#include "UpdateCheck.h"

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
    UpdateCheck updates{"Faverooo/ParametricNeuralAmpModelerPlugin", PNAM_BUILD_VERSION};
    juce::TextButton updateButton;
    juce::TextButton openButton{"Load model"};

    juce::Label amplifier;
    juce::Label status;
    juce::ToggleButton matchButton{"Level Match"};
    juce::TextButton relearnButton{"Relearn"};
    juce::Label matchStatus;
    juce::Label outputLabel{"output", "Output"};
    juce::Slider outputDial;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAttachment;
    juce::Rectangle<int> outputPanel;
    juce::Rectangle<int> headerRule;
    std::array<juce::Label, ParametricNAMProcessor::maximumControls> labels;
    std::array<juce::Slider, ParametricNAMProcessor::maximumControls> dials;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,
               ParametricNAMProcessor::maximumControls> attachments;
    std::unique_ptr<juce::FileChooser> chooser;
};
