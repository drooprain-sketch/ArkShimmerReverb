#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class ArkShimmerReverbAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                   private juce::Timer
{
public:
    explicit ArkShimmerReverbAudioProcessorEditor(ArkShimmerReverbAudioProcessor&);
    ~ArkShimmerReverbAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using APVTS = juce::AudioProcessorValueTreeState;
    using SliderAttachment = APVTS::SliderAttachment;
    using ComboBoxAttachment = APVTS::ComboBoxAttachment;
    using ButtonAttachment = APVTS::ButtonAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    struct Combo
    {
        juce::ComboBox box;
        juce::Label label;
        std::unique_ptr<ComboBoxAttachment> attachment;
    };

    void timerCallback() override;
    void addKnob(const juce::String& title, const juce::String& parameterID);
    void addCombo(const juce::String& title, const juce::String& parameterID);
    void styleSlider(juce::Slider& slider);
    void styleLabel(juce::Label& label);
    void styleCombo(juce::ComboBox& combo);

    ArkShimmerReverbAudioProcessor& processor;
    APVTS& apvts;

    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<Combo>> combos;

    juce::ToggleButton freezeButton;
    std::unique_ptr<ButtonAttachment> freezeAttachment;

    juce::TextButton clearButton { "CLEAR TAIL" };
    juce::Label titleLabel;
    juce::Label subtitleLabel;

    float visualPhase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArkShimmerReverbAudioProcessorEditor)
};
