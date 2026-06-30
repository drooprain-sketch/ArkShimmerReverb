#include "PluginProcessor.h"
#include "PluginEditor.h"

ArkShimmerReverbAudioProcessor::ArkShimmerReverbAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout ArkShimmerReverbAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto addFloat = [&params] (const juce::String& id,
                              const juce::String& name,
                              juce::NormalisableRange<float> range,
                              float defaultValue,
                              const juce::String& suffix = {})
    {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(id, 1), name, range, defaultValue,
            juce::AudioParameterFloatAttributes().withLabel(suffix)));
    };

    addFloat("mix", "Mix", { 0.0f, 100.0f, 0.01f }, 35.0f, "%");
    addFloat("decay", "Decay", { 0.8f, 60.0f, 0.01f, 0.45f }, 6.8f, "s");
    addFloat("size", "Size", { 0.0f, 100.0f, 0.01f }, 78.0f, "%");
    addFloat("shimmer", "Shimmer", { 0.0f, 100.0f, 0.01f }, 42.0f, "%");
    addFloat("tone", "Tone", { 0.0f, 100.0f, 0.01f }, 62.0f, "%");
    addFloat("mod", "Mod", { 0.0f, 100.0f, 0.01f }, 34.0f, "%");
    addFloat("width", "Width", { 0.0f, 150.0f, 0.01f }, 118.0f, "%");
    addFloat("duck", "Duck", { 0.0f, 100.0f, 0.01f }, 28.0f, "%");
    addFloat("lowcut", "Low Cut", { 20.0f, 800.0f, 0.01f, 0.36f }, 160.0f, "Hz");
    addFloat("highcut", "High Cut", { 2000.0f, 20000.0f, 0.01f, 0.48f }, 10800.0f, "Hz");
    addFloat("predelay", "Pre Delay", { 0.0f, 250.0f, 0.01f }, 32.0f, "ms");
    addFloat("input", "Input", { -24.0f, 12.0f, 0.01f }, 0.0f, "dB");
    addFloat("output", "Output", { -24.0f, 12.0f, 0.01f }, 0.0f, "dB");

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("mode", 1), "Mode",
        juce::StringArray { "Worship", "Cinematic", "Live", "Dark Ark", "Infinite" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("pitch", 1), "Pitch",
        juce::StringArray { "+12", "+7", "+12 +7", "-12", "Dual Detune", "Choir" }, 2));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("quality", 1), "Quality",
        juce::StringArray { "Live", "Studio" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("freeze", 1), "Freeze", false));

    return { params.begin(), params.end() };
}

void ArkShimmerReverbAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    currentSampleRate = sampleRate;

    preDelay.prepare(sampleRate);
    inputFilter.prepare(sampleRate);
    earlyDiffusion.prepare(sampleRate);
    reverb.prepare(sampleRate);
    shimmer.prepare(sampleRate);
    ducker.prepare(sampleRate);

    mixSmooth.reset(sampleRate, 0.025);
    outputSmooth.reset(sampleRate, 0.025);
    shimmerSmooth.reset(sampleRate, 0.040);

    clearTail();
    updateDSPParameters();
}

void ArkShimmerReverbAudioProcessor::releaseResources()
{
}

bool ArkShimmerReverbAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();
    const auto& mainIn = layouts.getMainInputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    return mainIn == mainOut;
}

void ArkShimmerReverbAudioProcessor::updateDSPParameters()
{
    const auto get = [this] (const char* id) -> float
    {
        if (auto* p = apvts.getRawParameterValue(id))
            return p->load();
        return 0.0f;
    };

    const float mix = get("mix") * 0.01f;
    const float decay = get("decay");
    const float size = get("size") * 0.01f;
    const float shimmerAmount = get("shimmer") * 0.01f;
    const float tone = get("tone") * 0.01f;
    const float mod = get("mod") * 0.01f;
    const float duck = get("duck") * 0.01f;
    const float lowCut = get("lowcut");
    const float highCut = get("highcut");
    const float preDelayMs = get("predelay");
    const float outputGain = ark::dbToGain(get("output"));

    const int mode = static_cast<int>(get("mode"));
    const int pitch = static_cast<int>(get("pitch"));
    const int quality = static_cast<int>(get("quality"));
    const bool freeze = get("freeze") > 0.5f;

    // Mode voicing: these are subtle offsets that keep the large controls useful.
    float voicedLowCut = lowCut;
    float voicedHighCut = highCut;
    float voicedTone = tone;
    float voicedMod = mod;

    if (mode == 1) // Cinematic
    {
        voicedLowCut = juce::jmax(lowCut, 120.0f);
        voicedHighCut = juce::jmin(highCut, 13500.0f);
        voicedMod = juce::jmin(1.0f, mod + 0.12f);
    }
    else if (mode == 2) // Live
    {
        voicedLowCut = juce::jmax(lowCut, 170.0f);
        voicedHighCut = juce::jmin(highCut, 12000.0f);
        voicedMod = juce::jmax(0.0f, mod - 0.10f);
    }
    else if (mode == 3) // Dark Ark
    {
        voicedLowCut = juce::jmax(lowCut, 110.0f);
        voicedHighCut = juce::jmin(highCut, 6500.0f);
        voicedTone = juce::jmin(voicedTone, 0.35f);
    }
    else if (mode == 4) // Infinite
    {
        voicedHighCut = juce::jmin(highCut, 14500.0f);
        voicedMod = juce::jmin(1.0f, mod + 0.18f);
    }

    preDelay.setDelayMs(preDelayMs);
    inputFilter.setCutoffs(voicedLowCut, voicedHighCut);
    earlyDiffusion.setDiffusion(juce::jmap(size, 0.25f, 0.85f));
    reverb.setParameters(decay, size, voicedTone, voicedMod, freeze, mode);
    shimmer.setParameters(pitch, voicedTone, quality == 0 || mode == 2);
    ducker.setAmount(duck);

    mixSmooth.setTargetValue(mix);
    outputSmooth.setTargetValue(outputGain);
    shimmerSmooth.setTargetValue(shimmerAmount);
}

void ArkShimmerReverbAudioProcessor::clearTail()
{
    preDelay.reset();
    inputFilter.reset();
    earlyDiffusion.reset();
    reverb.reset();
    shimmer.reset();
    ducker.reset();
    shimmerFeedbackL = 0.0f;
    shimmerFeedbackR = 0.0f;
}

void ArkShimmerReverbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    if (clearTailRequested.exchange(false))
        clearTail();

    updateDSPParameters();

    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    const bool stereo = buffer.getNumChannels() > 1;
    auto* left = buffer.getWritePointer(0);
    auto* right = stereo ? buffer.getWritePointer(1) : nullptr;

    const float inputGain = ark::dbToGain(apvts.getRawParameterValue("input")->load());
    const float widthPercent = apvts.getRawParameterValue("width")->load();
    const bool freeze = apvts.getRawParameterValue("freeze")->load() > 0.5f;

    for (int n = 0; n < buffer.getNumSamples(); ++n)
    {
        const float dryL = left[n];
        const float dryR = stereo ? right[n] : dryL;

        float l = dryL * inputGain;
        float r = dryR * inputGain;

        auto pre = preDelay.process(l, r);
        l = pre.first;
        r = pre.second;

        auto filtered = inputFilter.process(l, r);
        l = filtered.first;
        r = filtered.second;

        auto diffused = earlyDiffusion.process(l, r);
        l = diffused.first;
        r = diffused.second;

        const float shimmerAmount = shimmerSmooth.getNextValue();
        const float feedAmount = juce::jlimit(0.0f, 0.38f, shimmerAmount * 0.32f);
        const float directShimmerMix = shimmerAmount * 0.62f;

        const float reverbInputL = freeze ? 0.0f : l + shimmerFeedbackL * feedAmount;
        const float reverbInputR = freeze ? 0.0f : r + shimmerFeedbackR * feedAmount;

        auto wetBase = reverb.process(reverbInputL, reverbInputR);
        auto shimmerOut = shimmer.process(wetBase.first, wetBase.second);

        shimmerFeedbackL = ark::softLimit(shimmerOut.first);
        shimmerFeedbackR = ark::softLimit(shimmerOut.second);

        float wetL = wetBase.first + shimmerOut.first * directShimmerMix;
        float wetR = wetBase.second + shimmerOut.second * directShimmerMix;

        auto ducked = ducker.process(wetL, wetR, dryL, dryR);
        wetL = ducked.first;
        wetR = ducked.second;

        auto widened = ark::applyWidth(wetL, wetR, widthPercent);
        wetL = widened.first;
        wetR = widened.second;

        const float mix = mixSmooth.getNextValue();
        const float outGain = outputSmooth.getNextValue();

        float outL = dryL * (1.0f - mix) + wetL * mix;
        float outR = dryR * (1.0f - mix) + wetR * mix;

        outL = ark::softLimit(outL * outGain);
        outR = ark::softLimit(outR * outGain);

        left[n] = outL;
        if (stereo)
            right[n] = outR;
    }
}

void ArkShimmerReverbAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        if (auto xml = state.createXml())
            copyXmlToBinary(*xml, destData);
    }
}

void ArkShimmerReverbAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary(data, sizeInBytes))
    {
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessorEditor* ArkShimmerReverbAudioProcessor::createEditor()
{
    return new ArkShimmerReverbAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ArkShimmerReverbAudioProcessor();
}
