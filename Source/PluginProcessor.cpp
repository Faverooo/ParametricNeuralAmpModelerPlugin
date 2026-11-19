#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace
{
double sumSquares(const juce::AudioBuffer<float>& buffer)
{
    double energy = 0.0;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        const auto* samples = buffer.getReadPointer(channel);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            energy += double(samples[i]) * samples[i];
    }
    return energy;
}

juce::AudioProcessorValueTreeState::ParameterLayout makeParameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (size_t i = 0; i < ParametricNAMProcessor::maximumControls; ++i)
    {
        const auto id = ParametricNAMProcessor::controlId(i);
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id, 1}, "Control " + juce::String(static_cast<int>(i + 1)),
            juce::NormalisableRange<float>{0.0f, 1.0f}, 0.5f));
    }
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParametricNAMProcessor::outputTrimId, 1}, "Output",
        juce::NormalisableRange<float>{-24.0f, 24.0f, 0.1f}, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    return layout;
}
}

ParametricNAMProcessor::ParametricNAMProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "ParametricNAMState", makeParameters())
{
    for (size_t i = 0; i < maximumControls; ++i)
        controlValues[i] = parameters.getRawParameterValue(controlId(i));
    outputTrim = parameters.getRawParameterValue(outputTrimId);
}

juce::String ParametricNAMProcessor::controlId(size_t index)
{
    return "control" + juce::String(static_cast<int>(index + 1));
}

ParametricNAMProcessor::Model::Model(const std::filesystem::path& path)
    : channels{nam::ParametricModel::Load(path), nam::ParametricModel::Load(path)}
{
    for (auto& channel : channels)
    {
        if (channel.Audio().NumInputChannels() != 1 || channel.Audio().NumOutputChannels() != 1
            || channel.Controls().empty() || channel.Controls().size() > maximumControls)
            throw std::runtime_error("This VST3 expects a mono PNAM model with one to eight controls.");
    }
}

bool ParametricNAMProcessor::Model::prepare(double rate, int maximumBlockSize)
{
    controlsSet = false;
    if (maximumBlockSize <= 0 || rate != channels.front().Audio().GetExpectedSampleRate())
        return false;
    for (auto& channel : channels)
        channel.Audio().Reset(rate, maximumBlockSize);
    return true;
}

bool ParametricNAMProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    const auto output = layout.getMainOutputChannelSet();
    return output == layout.getMainInputChannelSet()
        && (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo());
}

void ParametricNAMProcessor::prepareToPlay(double sampleRate, int maximumBlockSize)
{
    const juce::ScopedLock lock(getCallbackLock());
    processingRate = sampleRate;
    blockCapacity = maximumBlockSize;
    modelReady = model && model->prepare(sampleRate, maximumBlockSize);
    outputGain.reset(sampleRate, 0.05);
    outputGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(outputTargetDb()));
}

float ParametricNAMProcessor::outputTargetDb() const
{
    return levelMatch.targetDb(outputTrim->load(std::memory_order_relaxed));
}

void ParametricNAMProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const juce::ScopedLock lock(getCallbackLock());
    if (!modelReady || buffer.getNumSamples() == 0)
        return;

    const bool measuring = levelMatch.measuring;
    const double dryEnergy = measuring ? sumSquares(buffer) : 0.0;
    std::array<float, maximumControls> values{};
    const auto& controls = model->channels.front().Controls();
    for (size_t i = 0; i < controls.size(); ++i)
        values[i] = std::lerp(controls[i].minimum, controls[i].maximum,
                              controlValues[i]->load(std::memory_order_relaxed));

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto& instance = model->channels[static_cast<size_t>(channel)];
        if (!model->controlsSet || values != model->lastValues)
            instance.SetControls(std::span<const float>(values.data(), controls.size()));

        for (int offset = 0; offset < buffer.getNumSamples();)
        {
            const int frames = std::min(blockCapacity, buffer.getNumSamples() - offset);
            auto* samples = buffer.getWritePointer(channel, offset);
            instance.Audio().process(&samples, &samples, frames);
            offset += frames;
        }
    }
    model->lastValues = values;
    model->controlsSet = true;

    if (measuring)
        levelMatch.measure(dryEnergy, sumSquares(buffer), buffer.getNumSamples(), buffer.getNumChannels(), processingRate);

    outputGain.setTargetValue(juce::Decibels::decibelsToGain(outputTargetDb()));
    outputGain.applyGain(buffer, buffer.getNumSamples());
}

void ParametricNAMProcessor::openModel(const juce::File& file)
{
    loadModel(file, true);
}

void ParametricNAMProcessor::loadModel(const juce::File& file, bool setDefaults)
{
    try
    {
        const auto path = file.getFullPathName().toStdString();
        auto next = std::make_unique<Model>(std::filesystem::path(std::u8string(path.begin(), path.end())));
        if (setDefaults)
        {
            const auto& channel = next->channels.front();
            if (const auto trim = channel.OutputTrimDb())
            {
                auto* output = parameters.getParameter(outputTrimId);
                output->setValueNotifyingHost(output->convertTo0to1(*trim));
            }
            const auto& controls = channel.Controls();
            for (size_t i = 0; i < controls.size(); ++i)
            {
                const auto& control = controls[i];
                auto* parameter = parameters.getParameter(controlId(i));
                parameter->setValueNotifyingHost((control.initial - control.minimum) / (control.maximum - control.minimum));
            }
        }
        for (;;)
        {
            double rate;
            int capacity;
            {
                const juce::ScopedLock lock(getCallbackLock());
                rate = processingRate;
                capacity = blockCapacity;
            }

            const bool ready = next->prepare(rate, capacity);
            const juce::ScopedLock lock(getCallbackLock());
            if (rate != processingRate || capacity != blockCapacity)
                continue;
            model.swap(next);
            levelMatch = {};
            outputGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(outputTargetDb()));
            modelReady = ready;
            modelFile = file;
            modelError.clear();
            break;
        }
    }
    catch (const std::exception& error)
    {
        std::unique_ptr<Model> previous;
        const juce::ScopedLock lock(getCallbackLock());
        modelError = "Could not open " + file.getFileName() + ". " + juce::String(error.what());
        if (!setDefaults)
        {
            previous.swap(model);
            modelReady = false;
            levelMatch = {};
            modelFile = file;
        }
    }
}

ParametricNAMProcessor::ModelInfo ParametricNAMProcessor::modelInfo()
{
    const juce::ScopedLock lock(getCallbackLock());
    ModelInfo info;
    info.error = modelError;
    info.modelReady = modelReady;
    info.match = levelMatch;
    if (model)
    {
        auto& channel = model->channels.front();
        info.amplifier = juce::String::fromUTF8(channel.Amplifier().c_str());
        info.controls = channel.Controls();
        info.sampleRate = channel.Audio().GetExpectedSampleRate();
        if (info.error.isEmpty() && processingRate > 0 && processingRate != info.sampleRate)
            info.error = "Set the audio device to " + juce::String(info.sampleRate, 0) + " Hz to hear this model.";
    }
    return info;
}

void ParametricNAMProcessor::enableLevelMatch(bool enabled)
{
    const juce::ScopedLock lock(getCallbackLock());
    if (modelReady)
        levelMatch.setEnabled(enabled);
}

void ParametricNAMProcessor::relearnLevelMatch()
{
    const juce::ScopedLock lock(getCallbackLock());
    if (modelReady)
        levelMatch.start();
}

void ParametricNAMProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    auto state = parameters.copyState();
    {
        const juce::ScopedLock lock(getCallbackLock());
        state.setProperty("modelFile", modelFile.getFullPathName(), nullptr);
        state.setProperty("matchEnabled", levelMatch.enabled, nullptr);
        if (levelMatch.correctionDb)
            state.setProperty("matchDb", *levelMatch.correctionDb, nullptr);
        else
            state.removeProperty("matchDb", nullptr);
    }
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destination);
}

void ParametricNAMProcessor::setStateInformation(const void* data, int bytes)
{
    auto xml = getXmlFromBinary(data, bytes);
    if (!xml || !xml->hasTagName(parameters.state.getType().toString()))
        return;

    auto state = juce::ValueTree::fromXml(*xml);
    const auto path = state.getProperty("modelFile").toString();
    if (!state.getChildWithProperty("id", outputTrimId).isValid())
    {
        juce::ValueTree trimState("PARAM");
        trimState.setProperty("id", outputTrimId, nullptr);
        trimState.setProperty("value", 0.0f, nullptr);
        state.appendChild(trimState, nullptr);
    }
    parameters.replaceState(state);
    if (path.isNotEmpty())
    {
        loadModel(juce::File(path), false);
        const juce::ScopedLock lock(getCallbackLock());
        if (model)
        {
            if (state.hasProperty("matchDb"))
            {
                const float db = static_cast<float>(state.getProperty("matchDb"));
                if (std::isfinite(db))
                    levelMatch.correctionDb = std::clamp(db, -LevelMatch::maximumCorrectionDb,
                                                           LevelMatch::maximumCorrectionDb);
            }
            levelMatch.setEnabled(static_cast<bool>(state.getProperty("matchEnabled", false)));
        }
    }
    else
    {
        std::unique_ptr<Model> previous;
        const juce::ScopedLock lock(getCallbackLock());
        previous.swap(model);
        modelReady = false;
        modelFile = juce::File{};
        levelMatch = {};
        outputGain.setCurrentAndTargetValue(1.0f);
        modelError.clear();
    }
}

juce::AudioProcessorEditor* ParametricNAMProcessor::createEditor()
{
    return new ParametricNAMEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ParametricNAMProcessor();
}
