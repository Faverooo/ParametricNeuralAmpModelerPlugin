#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr int editorWidth = 576;
const juce::Colour ink{0xfff0f4ff};
const juce::Colour muted{0xffa1adca};
const juce::Colour mint{0xff83f4ce};
const juce::Colour panel{0xff1a2438};
const juce::Colour edge{0xff33425e};
const std::array<juce::Colour, 4> dialColours{mint, juce::Colour(0xff8eb8ff),
                                             juce::Colour(0xffc5a6ff), juce::Colour(0xffffb68e)};

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
    const auto radius = std::min(38.0f, float(std::min(width, height)) / 2.0f - 5.0f);
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
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff374460), centre.x, disc.getY(),
                                          juce::Colour(0xff131d30), centre.x, disc.getBottom(), false));
    g.fillEllipse(disc);
    g.setColour(edge);
    g.drawEllipse(disc, 1.0f);
    const auto pointer = centre.getPointOnCircumference(radius - 17.0f, angle);
    g.setColour(accent);
    g.fillEllipse(juce::Rectangle<float>(5.0f, 5.0f).withCentre(pointer));
}

void ParametricNAMEditor::Theme::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                      const juce::Colour& colour, bool hover, bool down)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    auto fill = colour.withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.4f);
    g.setColour(fill.brighter(down ? 0.22f : hover ? 0.12f : 0.0f));
    g.fillRoundedRectangle(bounds, 9.0f);
    g.setColour(button.hasKeyboardFocus(true) ? mint : edge.withAlpha(0.65f));
    g.drawRoundedRectangle(bounds, 9.0f, 1.0f);
}

void ParametricNAMEditor::Theme::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                                  bool hover, bool down)
{
    auto area = button.getLocalBounds().toFloat();
    const juce::Rectangle<float> track(2.0f, area.getCentreY() - 8.0f, 30.0f, 16.0f);
    const bool active = button.getToggleState();
    const float opacity = button.isEnabled() ? 1.0f : 0.45f;
    g.setColour((active ? mint : edge).withAlpha(opacity).brighter((hover || down) ? 0.12f : 0.0f));
    g.fillRoundedRectangle(track, 8.0f);
    if (button.hasKeyboardFocus(true))
        g.drawRoundedRectangle(track.expanded(2.0f), 10.0f, 1.0f);
    g.setColour((active ? juce::Colour(0xff143329) : ink).withAlpha(opacity));
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
    addAndMakeVisible(openButton);
    addAndMakeVisible(identity);
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
    identity.setFont(face(12.0f, juce::Font::bold));
    identity.setColour(juce::Label::textColourId, mint);
    amplifier.setFont(face(27.0f, juce::Font::bold));
    amplifier.setColour(juce::Label::textColourId, ink);
    status.setColour(juce::Label::textColourId, juce::Colour(0xffffb68e));
    matchStatus.setColour(juce::Label::textColourId, mint);
    matchStatus.setJustificationType(juce::Justification::centredRight);
    openButton.setTooltip("Load a .pnam model");
    openButton.setColour(juce::TextButton::buttonColourId, mint);
    openButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff152a27));
    relearnButton.setColour(juce::TextButton::buttonColourId, panel);
    relearnButton.setColour(juce::TextButton::textColourOffId, ink);

    for (size_t i = 0; i < dials.size(); ++i)
    {
        auto& dial = dials[i];
        dial.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        dial.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 24);
        dial.setColour(juce::Slider::rotarySliderFillColourId, dialColours[i % dialColours.size()]);
        dial.setColour(juce::Slider::textBoxTextColourId, ink);
        dial.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff111a2b));
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
    setSize(editorWidth, 130);
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
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff202b43), 0, 0, juce::Colour(0xff101827),
                                          float(getWidth()), float(getHeight()), false));
    g.fillAll();
    for (size_t i = 0; i < info.controls.size(); ++i)
    {
        const auto card = cards[i].toFloat().reduced(0.5f);
        g.setColour(panel);
        g.fillRoundedRectangle(card, 13.0f);
        g.setColour(edge);
        g.drawRoundedRectangle(card, 13.0f, 1.0f);
    }
    if (!info.controls.empty())
    {
        g.setColour(edge);
        g.fillRect(20, getHeight() - 57, getWidth() - 40, 1);
    }
}

void ParametricNAMEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    auto header = area.removeFromTop(52);
    openButton.setBounds(header.removeFromRight(115).withSizeKeepingCentre(115, 34));
    identity.setBounds(header.removeFromTop(17));
    amplifier.setBounds(header.withTrimmedRight(8));
    status.setBounds(area.removeFromTop(26));

    auto toolbar = area.removeFromBottom(29);
    matchButton.setBounds(toolbar.removeFromLeft(130));
    relearnButton.setBounds(toolbar.removeFromRight(82));
    matchStatus.setBounds(toolbar.withTrimmedRight(10));
    area.removeFromBottom(16);

    const int controls = static_cast<int>(info.controls.size());
    const int rows = controls > 4 ? 2 : 1;
    const int rowHeight = area.getHeight() / rows;
    for (int start = 0; start < controls; start += 4)
    {
        auto row = area.removeFromTop(rowHeight).withTrimmedBottom(8);
        const int columns = std::min(4, controls - start);
        const int width = row.getWidth() / columns;
        for (int offset = 0; offset < columns; ++offset)
        {
            const auto index = static_cast<size_t>(start + offset);
            cards[index] = row.removeFromLeft(width).reduced(4, 0);
            auto column = cards[index].reduced(8, 8);
            labels[index].setBounds(column.removeFromTop(23));
            dials[index].setBounds(column);
        }
    }
}

void ParametricNAMEditor::timerCallback()
{
    auto next = processor.modelInfo();
    const auto previous = std::move(info);
    info = std::move(next);
    amplifier.setText(info.amplifier.isEmpty() ? "No model loaded" : info.amplifier, juce::dontSendNotification);
    status.setText(info.error, juce::dontSendNotification);

    if (info.controls.size() != previous.controls.size())
    {
        setSize(editorWidth, info.controls.empty() ? 130 : info.controls.size() > 4 ? 482 : 330);
        resized();
        repaint();
    }

    const auto& match = info.match;
    matchButton.setVisible(!info.controls.empty());
    relearnButton.setVisible(!info.controls.empty());
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
            matchText = "Output too quiet. Relearn.";
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
