#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr int editorWidth = 646;
constexpr int minimumWidth = 500;
constexpr int maximumWidth = 1100;
constexpr int maximumHeight = 720;
constexpr int emptyHeight = 122;

int minimumHeightFor(size_t controlCount)
{
    if (controlCount == 0)
        return emptyHeight;
    return controlCount <= 6 ? 326 : 420;
}
const juce::Colour surface{0xff191915};
const juce::Colour ink{0xfff2eee2};
const juce::Colour muted{0xffb7b0a0};
const juce::Colour amber{0xffffba69};
const juce::Colour panel{0xff25231d};
const juce::Colour edge{0xff514a3c};
const juce::Colour primary{0xffe7ddc8};

juce::Font face(float size, int style = juce::Font::plain)
{
    return juce::Font(juce::FontOptions(size, style));
}

void arc(juce::Graphics& g, juce::Point<float> centre, float radius, float start, float end)
{
    if (end <= start)
        return;
    juce::Path path;
    path.addCentredArc(centre.x, centre.y, radius, radius, 0, start, end, true);
    g.strokePath(path, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
}

void ParametricNAMEditor::Theme::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                                  float position, float start, float end, juce::Slider& slider)
{
    const auto centre = juce::Rectangle<float>(float(x), float(y), float(width), float(height)).getCentre();
    const auto radius = std::min(43.0f, float(std::min(width, height)) / 2.0f - 5.0f);
    if (radius < 12.0f)
        return;
    const auto accent = slider.findColour(juce::Slider::rotarySliderFillColourId);
    const auto angle = std::lerp(start, end, position);
    g.setColour(edge);
    arc(g, centre, radius, start, end);
    g.setColour(accent);
    arc(g, centre, radius, start, angle);

    const auto disc = juce::Rectangle<float>(2.0f * (radius - 9.0f), 2.0f * (radius - 9.0f))
                          .withCentre(centre);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff37352f), centre.x, disc.getY(),
                                          juce::Colour(0xff201f1a), centre.x, disc.getBottom(), false));
    g.fillEllipse(disc);
    g.setColour(edge);
    g.drawEllipse(disc, 1.0f);
    const auto pointer = centre.getPointOnCircumference(radius - 17.0f, angle);
    g.setColour(accent);
    g.fillEllipse(juce::Rectangle<float>(5.0f, 5.0f).withCentre(pointer));
    if (slider.hasKeyboardFocus(true))
    {
        g.setColour(accent);
        g.drawEllipse(disc.expanded(3.0f), 1.0f);
    }
}

void ParametricNAMEditor::Theme::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                      const juce::Colour& colour, bool hover, bool down)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    auto fill = colour.withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.4f);
    g.setColour(fill.brighter(down ? 0.22f : hover ? 0.12f : 0.0f));
    g.fillRoundedRectangle(bounds, 9.0f);
    g.setColour(button.hasKeyboardFocus(true) ? amber : edge.withAlpha(0.65f));
    g.drawRoundedRectangle(bounds, 9.0f, 1.0f);
}

void ParametricNAMEditor::Theme::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                                  bool hover, bool down)
{
    auto area = button.getLocalBounds().toFloat();
    const juce::Rectangle<float> track(2.0f, area.getCentreY() - 8.0f, 30.0f, 16.0f);
    const bool active = button.getToggleState();
    const float opacity = button.isEnabled() ? 1.0f : 0.45f;
    g.setColour((active ? amber : edge).withAlpha(opacity).brighter((hover || down) ? 0.12f : 0.0f));
    g.fillRoundedRectangle(track, 8.0f);
    if (button.hasKeyboardFocus(true))
        g.drawRoundedRectangle(track.expanded(2.0f), 10.0f, 1.0f);
    g.setColour((active ? juce::Colour(0xff2e2214) : ink).withAlpha(opacity));
    g.fillEllipse(track.getX() + (active ? 16.0f : 3.0f), track.getY() + 2.0f, 12.0f, 12.0f);
    g.setColour(ink.withAlpha(opacity));
    g.setFont(face(14.0f, juce::Font::bold));
    area.removeFromLeft(39.0f);
    g.drawText(button.getButtonText(), area, juce::Justification::centredLeft);
}

ParametricNAMEditor::ParametricNAMEditor(ParametricNAMProcessor& owner)
    : AudioProcessorEditor(owner), processor(owner)
{
    setLookAndFeel(&theme);
    addChildComponent(updateButton);
    updateButton.setColour(juce::TextButton::buttonColourId, panel);
    updateButton.setColour(juce::TextButton::textColourOffId, amber);
    updateButton.setTooltip("Open the latest VST3 release on GitHub");
    updateButton.onClick = [this] { updates.openReleasePage(); };
    addAndMakeVisible(openButton);
    addAndMakeVisible(outputLabel);
    addAndMakeVisible(outputDial);
    addAndMakeVisible(amplifier);
    addAndMakeVisible(status);
    addAndMakeVisible(matchButton);
    addAndMakeVisible(relearnButton);
    addAndMakeVisible(matchStatus);
    matchButton.onClick = [this]
    {
        processor.enableLevelMatch(matchButton.getToggleState());
        timerCallback();
    };
    relearnButton.onClick = [this]
    {
        processor.relearnLevelMatch();
        timerCallback();
    };
    openButton.onClick = [this] { chooseModel(); };

    amplifier.setFont(face(28.0f, juce::Font::bold));
    amplifier.setColour(juce::Label::textColourId, ink);
    status.setColour(juce::Label::textColourId, juce::Colour(0xffffb393));
    matchStatus.setColour(juce::Label::textColourId, muted);
    matchStatus.setJustificationType(juce::Justification::centred);
    openButton.setTooltip("Load a .pnam model");
    openButton.setColour(juce::TextButton::buttonColourId, primary);
    openButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff282116));
    relearnButton.setColour(juce::TextButton::buttonColourId, panel);
    relearnButton.setColour(juce::TextButton::textColourOffId, ink);

    outputLabel.setFont(face(15.0f, juce::Font::bold));
    outputLabel.setColour(juce::Label::textColourId, ink);
    outputLabel.setJustificationType(juce::Justification::centred);
    outputDial.setTitle("Output volume");
    outputDial.setTooltip("Manual output gain. Turn off Level Match to adjust.");
    outputDial.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    outputDial.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 86, 24);
    outputDial.setColour(juce::Slider::rotarySliderFillColourId, amber);
    outputDial.setColour(juce::Slider::textBoxTextColourId, ink);
    outputDial.setColour(juce::Slider::textBoxBackgroundColourId, surface);
    outputDial.setColour(juce::Slider::textBoxOutlineColourId, edge);
    outputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, ParametricNAMProcessor::outputTrimId, outputDial);
    outputDial.textFromValueFunction = [](double db)
    {
        return juce::String(db > 0 ? "+" : "") + juce::String(db, 1) + " dB";
    };
    outputDial.valueFromTextFunction = [](const juce::String& text) { return text.getDoubleValue(); };
    outputDial.setDoubleClickReturnValue(true, 0.0);
    outputDial.updateText();

    for (size_t i = 0; i < dials.size(); ++i)
    {
        auto& dial = dials[i];
        dial.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        dial.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 24);
        dial.setColour(juce::Slider::rotarySliderFillColourId, primary);
        dial.setColour(juce::Slider::textBoxTextColourId, ink);
        dial.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff201e18));
        dial.setColour(juce::Slider::textBoxOutlineColourId, edge);
        addAndMakeVisible(dial);
        labels[i].setFont(face(14.0f, juce::Font::bold));
        labels[i].setColour(juce::Label::textColourId, ink);
        labels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(labels[i]);

        attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, ParametricNAMProcessor::controlId(i), dial);
        dial.textFromValueFunction = [this, i](double normalized)
        {
            if (i >= info.controls.size())
                return juce::String{};
            const auto& control = info.controls[i];
            return juce::String(std::lerp(double(control.minimum), double(control.maximum), normalized), 2);
        };
        dial.valueFromTextFunction = [this, i](const juce::String& text)
        {
            if (i >= info.controls.size())
                return 0.0;
            const auto& control = info.controls[i];
            return juce::jlimit(0.0, 1.0, (text.getDoubleValue() - control.minimum)
                                          / (control.maximum - control.minimum));
        };
    }
    setResizable(true, true);
    setResizeLimits(minimumWidth, emptyHeight, maximumWidth, maximumHeight);
    setSize(editorWidth, emptyHeight);
    timerCallback();
    startTimerHz(6);
}

ParametricNAMEditor::~ParametricNAMEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void ParametricNAMEditor::paint(juce::Graphics& g)
{
    g.fillAll(surface);
    g.setColour(edge);
    g.fillRect(headerRule);
    if (!outputPanel.isEmpty())
    {
        g.setColour(panel);
        g.fillRoundedRectangle(outputPanel.toFloat(), 13.0f);
    }
}

void ParametricNAMEditor::resized()
{
    auto area = getLocalBounds().reduced(22, 16);
    auto header = area.removeFromTop(44);
    openButton.setBounds(header.removeFromRight(110).withSizeKeepingCentre(110, 34));
    if (updateButton.isVisible())
    {
        header.removeFromRight(8);
        updateButton.setBounds(header.removeFromRight(104).withSizeKeepingCentre(104, 34));
    }
    amplifier.setBounds(header.withTrimmedRight(8));
    headerRule = area.removeFromTop(7).withTrimmedTop(6);
    status.setBounds(area.removeFromBottom(info.controls.empty() ? 36 : 25));
    area.removeFromTop(14);
    outputPanel = {};
    if (info.controls.empty())
        return;

    outputPanel = area.removeFromRight(getWidth() < 600 ? 151 : 171);
    auto output = outputPanel.reduced(14, 10);
    outputLabel.setBounds(output.removeFromTop(23));
    outputDial.setBounds(output.removeFromTop(std::clamp(output.getHeight() - 76, 78, 106)));
    matchButton.setBounds(output.removeFromTop(28));
    matchStatus.setBounds(output.removeFromTop(22));
    relearnButton.setBounds(output.removeFromTop(26).withSizeKeepingCentre(84, 26));
    area.removeFromRight(12);

    const int controls = static_cast<int>(info.controls.size());
    const int availableColumns = std::clamp(area.getWidth() / 92, 1, 4);
    const int columnsPerRow = controls == 4 && availableColumns == 3 ? 2 : availableColumns;
    const int rows = (controls + columnsPerRow - 1) / columnsPerRow;
    const int rowHeight = area.getHeight() / rows;
    for (int start = 0; start < controls; start += columnsPerRow)
    {
        auto row = area.removeFromTop(rowHeight).withTrimmedBottom(8);
        row = row.withSizeKeepingCentre(row.getWidth(), std::min(154, row.getHeight()));
        const int columns = std::min(columnsPerRow, controls - start);
        const int width = std::min(132, row.getWidth() / columns);
        row = row.withSizeKeepingCentre(width * columns, row.getHeight());
        for (int offset = 0; offset < columns; ++offset)
        {
            const auto index = static_cast<size_t>(start + offset);
            auto column = row.removeFromLeft(width).reduced(4, 3);
            labels[index].setBounds(column.removeFromTop(23));
            dials[index].setBounds(column);
        }
    }
}

void ParametricNAMEditor::timerCallback()
{
    if (!updateButton.isVisible())
    {
        const auto version = updates.availableVersion();
        if (version.isNotEmpty())
        {
            updateButton.setButtonText("Update " + version);
            updateButton.setVisible(true);
            resized();
        }
    }
    auto next = processor.modelInfo();
    const auto previous = std::move(info);
    info = std::move(next);
    amplifier.setText(info.amplifier.isEmpty() ? "ParametricNAM" : info.amplifier, juce::dontSendNotification);
    status.setText(info.error.isEmpty() && info.controls.empty() ? juce::String("Load a .pnam model") : info.error, juce::dontSendNotification);

    if (info.controls.size() != previous.controls.size())
    {
        const auto minimumHeight = minimumHeightFor(info.controls.size());
        setResizeLimits(minimumWidth, minimumHeight, maximumWidth, maximumHeight);
        const auto preferredHeight = previous.controls.empty() && !info.controls.empty()
            ? (info.controls.size() > 4 ? 442 : 346) : getHeight();
        setSize(getWidth(), std::max(minimumHeight, preferredHeight));
        resized();
        repaint();
    }

    const auto& match = info.match;
    const bool hasModel = !info.controls.empty();
    const bool manual = info.modelReady && !match.enabled;
    outputLabel.setVisible(hasModel);
    outputDial.setVisible(hasModel);
    outputDial.setEnabled(manual);
    outputDial.setAlpha(manual ? 1.0f : 0.38f);
    matchButton.setVisible(hasModel);
    relearnButton.setVisible(hasModel && match.enabled);
    matchStatus.setVisible(!info.controls.empty());
    matchButton.setEnabled(info.modelReady);
    matchButton.setToggleState(match.enabled, juce::dontSendNotification);
    relearnButton.setEnabled(info.modelReady && match.enabled && !match.measuring);
    juce::String matchText;
    if (info.modelReady && match.enabled)
    {
        if (match.measuring)
            matchText = "Measuring " + juce::String(match.activeSeconds, 1) + " / 5 s";
        else if (match.noOutput)
            matchText = "Output too quiet";
        else if (match.correctionDb)
            matchText = juce::String(*match.correctionDb > 0 ? "+" : "") + juce::String(*match.correctionDb, 1) + " dB";
    }
    matchStatus.setText(matchText, juce::dontSendNotification);

    for (size_t i = 0; i < dials.size(); ++i)
    {
        const bool available = i < info.controls.size();
        labels[i].setVisible(available);
        dials[i].setVisible(available);
        if (available)
        {
            const auto label = juce::String::fromUTF8(info.controls[i].label.c_str());
            labels[i].setText(label, juce::dontSendNotification);
            dials[i].setTitle(label);
            const auto& control = info.controls[i];
            if (i >= previous.controls.size() || control.minimum != previous.controls[i].minimum
                || control.maximum != previous.controls[i].maximum)
                dials[i].updateText();
        }
    }
}

void ParametricNAMEditor::chooseModel()
{
    chooser = std::make_unique<juce::FileChooser>("Open PNAM model", juce::File{}, "*.pnam");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                         [safe = juce::Component::SafePointer<ParametricNAMEditor>(this)](const juce::FileChooser& dialog)
    {
        if (safe != nullptr && dialog.getResult() != juce::File{})
        {
            safe->processor.openModel(dialog.getResult());
            safe->timerCallback();
        }
    });
}
