#pragma once

#include <JuceHeader.h>
#include "DSP/ArkDSP.h"

class ArkShimmerReverbAudioProcessor final : public juce::AudioProcessor
{
public:
    ArkShimmerReverbAudioProcessor();
    ~ArkShimmerReverbAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 20.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    void requestClearTail() noexcept { clearTailRequested.store(true); }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    void updateDSPParameters();
    void clearTail();

    juce::AudioProcessorValueTreeState apvts;

    ark::PreDelay preDelay;
    ark::StereoToneFilter inputFilter;
    ark::StereoDiffuser earlyDiffusion;
    ark::FDNReverb reverb;
    ark::ShimmerEngine shimmer;
    ark::Ducker ducker;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> shimmerSmooth;

    std::atomic<bool> clearTailRequested { false };

    float shimmerFeedbackL = 0.0f;
    float shimmerFeedbackR = 0.0f;
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArkShimmerReverbAudioProcessor)
};
