#include "PluginEditor.h"

namespace
{
    juce::Colour bgTop()       { return juce::Colour::fromRGB(6, 14, 28); }
    juce::Colour bgBottom()    { return juce::Colour::fromRGB(10, 8, 18); }
    juce::Colour panel()       { return juce::Colour::fromRGB(15, 25, 44).withAlpha(0.86f); }
    juce::Colour gold()        { return juce::Colour::fromRGB(229, 180, 80); }
    juce::Colour softGold()    { return juce::Colour::fromRGB(255, 214, 128); }
    juce::Colour blueGlow()    { return juce::Colour::fromRGB(87, 160, 255); }
    juce::Colour textMain()    { return juce::Colour::fromRGB(238, 240, 245); }
    juce::Colour textDim()     { return juce::Colour::fromRGB(156, 169, 190); }
}

ArkShimmerReverbAudioProcessorEditor::ArkShimmerReverbAudioProcessorEditor(ArkShimmerReverbAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p), apvts(p.getAPVTS())
{
    setSize(820, 520);
    setResizable(true, true);
    setResizeLimits(720, 470, 1200, 760);

    titleLabel.setText("ARK", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setColour(juce::Label::textColourId, softGold());
    titleLabel.setFont(juce::Font(34.0f, juce::Font::bold));
    addAndMakeVisible(titleLabel);

    subtitleLabel.setText("Shimmer Reverb  •  Celestial Reverb & Shimmer Engine", juce::dontSendNotification);
    subtitleLabel.setJustificationType(juce::Justification::centredLeft);
    subtitleLabel.setColour(juce::Label::textColourId, textDim());
    subtitleLabel.setFont(juce::Font(14.0f, juce::Font::plain));
    addAndMakeVisible(subtitleLabel);

    addKnob("MIX", "mix");
    addKnob("DECAY", "decay");
    addKnob("SIZE", "size");
    addKnob("SHIMMER", "shimmer");
    addKnob("TONE", "tone");
    addKnob("MOD", "mod");
    addKnob("WIDTH", "width");
    addKnob("DUCK", "duck");
    addKnob("LOW CUT", "lowcut");
    addKnob("HIGH CUT", "highcut");
    addKnob("PRE DELAY", "predelay");
    addKnob("OUTPUT", "output");

    addCombo("MODE", "mode");
    addCombo("PITCH", "pitch");
    addCombo("QUALITY", "quality");

    freezeButton.setButtonText("FREEZE");
    freezeButton.setColour(juce::ToggleButton::textColourId, textMain());
    freezeButton.setColour(juce::ToggleButton::tickColourId, gold());
    freezeButton.setColour(juce::ToggleButton::tickDisabledColourId, textDim());
    freezeAttachment = std::make_unique<ButtonAttachment>(apvts, "freeze", freezeButton);
    addAndMakeVisible(freezeButton);

    clearButton.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(38, 47, 66));
    clearButton.setColour(juce::TextButton::buttonOnColourId, gold());
    clearButton.setColour(juce::TextButton::textColourOffId, textMain());
    clearButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    clearButton.onClick = [this] { processor.requestClearTail(); };
    addAndMakeVisible(clearButton);

    startTimerHz(30);
}

void ArkShimmerReverbAudioProcessorEditor::addKnob(const juce::String& title, const juce::String& parameterID)
{
    auto knob = std::make_unique<Knob>();
    styleSlider(knob->slider);
    styleLabel(knob->label);
    knob->label.setText(title, juce::dontSendNotification);
    knob->attachment = std::make_unique<SliderAttachment>(apvts, parameterID, knob->slider);

    addAndMakeVisible(knob->slider);
    addAndMakeVisible(knob->label);
    knobs.push_back(std::move(knob));
}

void ArkShimmerReverbAudioProcessorEditor::addCombo(const juce::String& title, const juce::String& parameterID)
{
    auto combo = std::make_unique<Combo>();
    styleCombo(combo->box);
    styleLabel(combo->label);
    combo->label.setText(title, juce::dontSendNotification);
    combo->attachment = std::make_unique<ComboBoxAttachment>(apvts, parameterID, combo->box);

    addAndMakeVisible(combo->box);
    addAndMakeVisible(combo->label);
    combos.push_back(std::move(combo));
}

void ArkShimmerReverbAudioProcessorEditor::styleSlider(juce::Slider& slider)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 20);
    slider.setColour(juce::Slider::rotarySliderFillColourId, gold());
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour::fromRGB(55, 72, 102));
    slider.setColour(juce::Slider::thumbColourId, softGold());
    slider.setColour(juce::Slider::textBoxTextColourId, textMain());
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void ArkShimmerReverbAudioProcessorEditor::styleLabel(juce::Label& label)
{
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, textDim());
    label.setFont(juce::Font(12.0f, juce::Font::bold));
}

void ArkShimmerReverbAudioProcessorEditor::styleCombo(juce::ComboBox& combo)
{
    combo.setColour(juce::ComboBox::backgroundColourId, juce::Colour::fromRGB(21, 32, 52));
    combo.setColour(juce::ComboBox::outlineColourId, juce::Colour::fromRGB(70, 88, 125));
    combo.setColour(juce::ComboBox::textColourId, textMain());
    combo.setColour(juce::ComboBox::arrowColourId, gold());
    combo.setColour(juce::PopupMenu::backgroundColourId, juce::Colour::fromRGB(12, 18, 30));
    combo.setColour(juce::PopupMenu::textColourId, textMain());
    combo.setJustificationType(juce::Justification::centred);
}

void ArkShimmerReverbAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient bg(bgTop(), bounds.getCentreX(), bounds.getY(), bgBottom(), bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(bounds.reduced(4.0f), 18.0f);

    // Abstract ark/rainbow arc.
    auto arcArea = bounds.reduced(32.0f, 34.0f).withHeight(148.0f);
    juce::Path arc;
    arc.addCentredArc(arcArea.getCentreX(), arcArea.getBottom() + 52.0f,
                      arcArea.getWidth() * 0.43f, arcArea.getHeight() * 0.82f,
                      0.0f,
                      juce::MathConstants<float>::pi * 1.06f,
                      juce::MathConstants<float>::pi * 1.94f,
                      true);

    g.setColour(blueGlow().withAlpha(0.20f));
    g.strokePath(arc, juce::PathStrokeType(16.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(gold().withAlpha(0.78f));
    g.strokePath(arc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Animated shimmer visualizer.
    auto visual = getLocalBounds().reduced(34, 86).removeFromTop(105).toFloat();
    g.setColour(panel());
    g.fillRoundedRectangle(visual, 14.0f);
    g.setColour(juce::Colour::fromRGB(80, 96, 132).withAlpha(0.35f));
    g.drawRoundedRectangle(visual, 14.0f, 1.0f);

    juce::Path wave;
    const int points = 120;
    for (int i = 0; i < points; ++i)
    {
        const float x = juce::jmap(static_cast<float>(i), 0.0f, static_cast<float>(points - 1), visual.getX() + 16.0f, visual.getRight() - 16.0f);
        const float t = static_cast<float>(i) / static_cast<float>(points - 1);
        const float envelope = std::sin(juce::MathConstants<float>::pi * t);
        const float y = visual.getCentreY()
            + std::sin((t * 9.0f + visualPhase) * juce::MathConstants<float>::twoPi) * envelope * 17.0f
            + std::sin((t * 19.0f - visualPhase * 0.71f) * juce::MathConstants<float>::twoPi) * envelope * 6.0f;

        if (i == 0) wave.startNewSubPath(x, y);
        else wave.lineTo(x, y);
    }

    g.setColour(blueGlow().withAlpha(0.25f));
    g.strokePath(wave, juce::PathStrokeType(7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(softGold().withAlpha(0.88f));
    g.strokePath(wave, juce::PathStrokeType(2.1f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Control panel.
    auto controlPanel = getLocalBounds().reduced(24).removeFromBottom(getHeight() - 230).toFloat();
    g.setColour(panel().withAlpha(0.72f));
    g.fillRoundedRectangle(controlPanel, 18.0f);
    g.setColour(juce::Colour::fromRGB(73, 91, 124).withAlpha(0.28f));
    g.drawRoundedRectangle(controlPanel, 18.0f, 1.0f);
}

void ArkShimmerReverbAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(30);
    auto header = area.removeFromTop(56);
    titleLabel.setBounds(header.removeFromLeft(120));
    subtitleLabel.setBounds(header);

    area.removeFromTop(126);
    auto controlArea = area.reduced(10, 4);

    auto topRow = controlArea.removeFromTop(112);
    auto secondRow = controlArea.removeFromTop(112);
    auto bottomRow = controlArea.removeFromTop(72);

    const int knobW = topRow.getWidth() / 8;
    for (int i = 0; i < 8 && i < static_cast<int>(knobs.size()); ++i)
    {
        auto cell = topRow.removeFromLeft(knobW).reduced(4);
        knobs[static_cast<size_t>(i)]->label.setBounds(cell.removeFromTop(18));
        knobs[static_cast<size_t>(i)]->slider.setBounds(cell);
    }

    const int knobW2 = secondRow.getWidth() / 4;
    for (int i = 8; i < 12 && i < static_cast<int>(knobs.size()); ++i)
    {
        auto cell = secondRow.removeFromLeft(knobW2).reduced(10, 4);
        knobs[static_cast<size_t>(i)]->label.setBounds(cell.removeFromTop(18));
        knobs[static_cast<size_t>(i)]->slider.setBounds(cell);
    }

    const int comboW = bottomRow.getWidth() / 5;
    for (int i = 0; i < 3 && i < static_cast<int>(combos.size()); ++i)
    {
        auto cell = bottomRow.removeFromLeft(comboW).reduced(8);
        combos[static_cast<size_t>(i)]->label.setBounds(cell.removeFromTop(18));
        combos[static_cast<size_t>(i)]->box.setBounds(cell.removeFromTop(34));
    }

    auto freezeCell = bottomRow.removeFromLeft(comboW).reduced(10);
    freezeButton.setBounds(freezeCell.removeFromTop(52));

    auto clearCell = bottomRow.reduced(10);
    clearButton.setBounds(clearCell.removeFromTop(38));
}

void ArkShimmerReverbAudioProcessorEditor::timerCallback()
{
    visualPhase += 0.006f;
    if (visualPhase >= 1.0f)
        visualPhase -= 1.0f;

    repaint(getLocalBounds().reduced(34, 86).removeFromTop(105));
}
