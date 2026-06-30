#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>
#include <cmath>

namespace ark
{
static constexpr int kFDNLines = 8;

inline float dbToGain(float dB) noexcept
{
    return std::pow(10.0f, dB / 20.0f);
}

inline float softLimit(float x) noexcept
{
    // Transparent enough for normal levels, protective on runaway feedback.
    return std::tanh(x * 0.85f) / 0.85f;
}

inline float safeFrequency(float hz, double sampleRate) noexcept
{
    return juce::jlimit(10.0f, static_cast<float>(sampleRate * 0.45), hz);
}

class FractionalDelayLine
{
public:
    void prepare(double sr, float maxDelayMs)
    {
        sampleRate = sr;
        const int size = static_cast<int>(std::ceil(sr * maxDelayMs * 0.001f)) + 8;
        buffer.assign(static_cast<size_t>(juce::jmax(16, size)), 0.0f);
        writeIndex = 0;
    }

    void reset()
    {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writeIndex = 0;
    }

    void push(float x) noexcept
    {
        if (buffer.empty()) return;
        buffer[static_cast<size_t>(writeIndex)] = x;
        writeIndex = (writeIndex + 1) % static_cast<int>(buffer.size());
    }

    float read(float delaySamples) const noexcept
    {
        if (buffer.empty()) return 0.0f;

        const auto size = static_cast<int>(buffer.size());
        delaySamples = juce::jlimit(1.0f, static_cast<float>(size - 4), delaySamples);

        float readPos = static_cast<float>(writeIndex) - delaySamples;
        while (readPos < 0.0f)
            readPos += static_cast<float>(size);

        const int i0 = static_cast<int>(std::floor(readPos)) % size;
        const int i1 = (i0 + 1) % size;
        const float frac = readPos - std::floor(readPos);

        return buffer[static_cast<size_t>(i0)] * (1.0f - frac)
             + buffer[static_cast<size_t>(i1)] * frac;
    }

private:
    double sampleRate = 44100.0;
    std::vector<float> buffer;
    int writeIndex = 0;
};

class PreDelay
{
public:
    void prepare(double sr, float maxDelayMs = 300.0f)
    {
        sampleRate = sr;
        left.prepare(sr, maxDelayMs);
        right.prepare(sr, maxDelayMs);
    }

    void reset()
    {
        left.reset();
        right.reset();
    }

    void setDelayMs(float ms) noexcept
    {
        delaySamples = juce::jlimit(0.0f, static_cast<float>(sampleRate * 0.290), ms * static_cast<float>(sampleRate) * 0.001f);
    }

    std::pair<float, float> process(float l, float r) noexcept
    {
        const float outL = delaySamples < 1.0f ? l : left.read(delaySamples);
        const float outR = delaySamples < 1.0f ? r : right.read(delaySamples);
        left.push(l);
        right.push(r);
        return { outL, outR };
    }

private:
    double sampleRate = 44100.0;
    float delaySamples = 0.0f;
    FractionalDelayLine left, right;
};

class OnePoleLowPass
{
public:
    void prepare(double sr)
    {
        sampleRate = sr;
        reset();
        setCutoff(12000.0f);
    }

    void reset() noexcept { z = 0.0f; }

    void setCutoff(float hz) noexcept
    {
        hz = safeFrequency(hz, sampleRate);
        const float x = std::exp(-2.0f * juce::MathConstants<float>::pi * hz / static_cast<float>(sampleRate));
        a = 1.0f - x;
    }

    float process(float x) noexcept
    {
        z += a * (x - z);
        return z;
    }

private:
    double sampleRate = 44100.0;
    float a = 0.1f;
    float z = 0.0f;
};

class OnePoleHighPass
{
public:
    void prepare(double sr)
    {
        sampleRate = sr;
        low.prepare(sr);
        setCutoff(80.0f);
    }

    void reset() noexcept { low.reset(); }

    void setCutoff(float hz) noexcept { low.setCutoff(hz); }

    float process(float x) noexcept
    {
        return x - low.process(x);
    }

private:
    double sampleRate = 44100.0;
    OnePoleLowPass low;
};

class StereoToneFilter
{
public:
    void prepare(double sr)
    {
        sampleRate = sr;
        hpL.prepare(sr); hpR.prepare(sr);
        lpL.prepare(sr); lpR.prepare(sr);
    }

    void reset()
    {
        hpL.reset(); hpR.reset(); lpL.reset(); lpR.reset();
    }

    void setCutoffs(float lowHz, float highHz)
    {
        hpL.setCutoff(lowHz); hpR.setCutoff(lowHz);
        lpL.setCutoff(highHz); lpR.setCutoff(highHz);
    }

    std::pair<float, float> process(float l, float r) noexcept
    {
        l = lpL.process(hpL.process(l));
        r = lpR.process(hpR.process(r));
        return { l, r };
    }

private:
    double sampleRate = 44100.0;
    OnePoleHighPass hpL, hpR;
    OnePoleLowPass lpL, lpR;
};

class Allpass
{
public:
    void prepare(double sr, float delayMs)
    {
        delay.prepare(sr, delayMs + 5.0f);
        setDelayMs(delayMs);
    }

    void reset() { delay.reset(); }

    void setDelayMs(float ms)
    {
        delaySamples = ms * static_cast<float>(sampleRate) * 0.001f;
    }

    void setSampleRate(double sr) { sampleRate = sr; }

    void setFeedback(float g) noexcept { feedback = juce::jlimit(0.0f, 0.85f, g); }

    float process(float x) noexcept
    {
        const float delayed = delay.read(delaySamples);
        const float y = -feedback * x + delayed;
        delay.push(x + feedback * y);
        return y;
    }

private:
    double sampleRate = 44100.0;
    float delaySamples = 100.0f;
    float feedback = 0.55f;
    FractionalDelayLine delay;
};

class StereoDiffuser
{
public:
    void prepare(double sr)
    {
        sampleRate = sr;
        const std::array<float, 4> lDelays { 4.7f, 8.9f, 13.1f, 21.7f };
        const std::array<float, 4> rDelays { 5.3f, 9.7f, 15.3f, 23.9f };

        for (int i = 0; i < 4; ++i)
        {
            left[static_cast<size_t>(i)].setSampleRate(sr);
            right[static_cast<size_t>(i)].setSampleRate(sr);
            left[static_cast<size_t>(i)].prepare(sr, lDelays[static_cast<size_t>(i)]);
            right[static_cast<size_t>(i)].prepare(sr, rDelays[static_cast<size_t>(i)]);
        }
    }

    void reset()
    {
        for (auto& a : left) a.reset();
        for (auto& a : right) a.reset();
    }

    void setDiffusion(float amount01)
    {
        const float g = juce::jmap(amount01, 0.25f, 0.72f);
        for (auto& a : left) a.setFeedback(g);
        for (auto& a : right) a.setFeedback(g);
    }

    std::pair<float, float> process(float l, float r) noexcept
    {
        for (auto& a : left) l = a.process(l);
        for (auto& a : right) r = a.process(r);
        return { l, r };
    }

private:
    double sampleRate = 44100.0;
    std::array<Allpass, 4> left, right;
};

class DelayPitchShifter
{
public:
    void prepare(double sr)
    {
        sampleRate = sr;
        delay.prepare(sr, 140.0f);
        reset();
    }

    void reset()
    {
        delay.reset();
        phaseA = 0.0f;
        phaseB = 0.5f;
    }

    void setRatio(float newRatio, bool liveMode)
    {
        ratio = juce::jlimit(0.25f, 4.0f, newRatio);
        minDelaySamples = (liveMode ? 8.0f : 16.0f) * static_cast<float>(sampleRate) * 0.001f;
        sweepSamples = (liveMode ? 42.0f : 70.0f) * static_cast<float>(sampleRate) * 0.001f;
        phaseIncrement = std::abs(1.0f - ratio) / juce::jmax(64.0f, sweepSamples);
        if (phaseIncrement < 0.000001f)
            phaseIncrement = 0.0f;
    }

    float process(float x) noexcept
    {
        delay.push(x);

        if (phaseIncrement == 0.0f)
            return delay.read(minDelaySamples + sweepSamples * 0.5f);

        const float yA = readVoice(phaseA);
        const float yB = readVoice(phaseB);

        phaseA += phaseIncrement;
        phaseB += phaseIncrement;
        if (phaseA >= 1.0f) phaseA -= 1.0f;
        if (phaseB >= 1.0f) phaseB -= 1.0f;

        return yA + yB;
    }

private:
    float readVoice(float phase) const noexcept
    {
        const float window = 0.5f - 0.5f * std::cos(juce::MathConstants<float>::twoPi * phase);
        const bool pitchUp = ratio >= 1.0f;
        const float ramp = pitchUp ? (1.0f - phase) : phase;
        const float d = minDelaySamples + ramp * sweepSamples;
        return delay.read(d) * window;
    }

    double sampleRate = 44100.0;
    FractionalDelayLine delay;
    float ratio = 2.0f;
    float minDelaySamples = 256.0f;
    float sweepSamples = 2048.0f;
    float phaseIncrement = 0.0f;
    float phaseA = 0.0f, phaseB = 0.5f;
};

class ShimmerEngine
{
public:
    enum PitchMode
    {
        OctaveUp = 0,
        FifthUp,
        OctaveAndFifth,
        OctaveDown,
        DualDetune,
        Choir
    };

    void prepare(double sr)
    {
        sampleRate = sr;
        for (auto& v : voicesL) v.prepare(sr);
        for (auto& v : voicesR) v.prepare(sr);
        toneL.prepare(sr); toneR.prepare(sr);
        reset();
    }

    void reset()
    {
        for (auto& v : voicesL) v.reset();
        for (auto& v : voicesR) v.reset();
        toneL.reset(); toneR.reset();
    }

    void setParameters(int pitchMode, float tone01, bool liveMode)
    {
        mode = static_cast<PitchMode>(juce::jlimit(0, 5, pitchMode));
        const float shimmerHighCut = juce::jmap(tone01, 0.0f, 1.0f, 4500.0f, 16000.0f);
        toneL.setCutoff(shimmerHighCut);
        toneR.setCutoff(shimmerHighCut);

        auto ratioForSemis = [] (float semitones) { return std::pow(2.0f, semitones / 12.0f); };

        switch (mode)
        {
            case OctaveUp:
                setVoiceRatios(ratioForSemis(12.0f), ratioForSemis(12.0f), liveMode);
                break;
            case FifthUp:
                setVoiceRatios(ratioForSemis(7.0f), ratioForSemis(7.0f), liveMode);
                break;
            case OctaveAndFifth:
                setVoiceRatios(ratioForSemis(12.0f), ratioForSemis(7.0f), liveMode);
                break;
            case OctaveDown:
                setVoiceRatios(ratioForSemis(-12.0f), ratioForSemis(-12.0f), liveMode);
                break;
            case DualDetune:
                setVoiceRatios(ratioForSemis(0.08f), ratioForSemis(-0.08f), liveMode);
                break;
            case Choir:
                setVoiceRatios(ratioForSemis(12.0f + 0.07f), ratioForSemis(7.0f - 0.06f), liveMode);
                break;
        }
    }

    std::pair<float, float> process(float l, float r) noexcept
    {
        float aL = voicesL[0].process(l);
        float bL = voicesL[1].process(l);
        float aR = voicesR[0].process(r);
        float bR = voicesR[1].process(r);

        float mixA = 0.55f;
        float mixB = 0.45f;
        if (mode == OctaveUp || mode == FifthUp || mode == OctaveDown)
        {
            mixA = 0.70f;
            mixB = 0.30f;
        }
        else if (mode == DualDetune)
        {
            mixA = 0.50f;
            mixB = 0.50f;
        }

        const float outL = toneL.process(aL * mixA + bL * mixB);
        const float outR = toneR.process(aR * mixB + bR * mixA);
        return { outL, outR };
    }

private:
    void setVoiceRatios(float ratioA, float ratioB, bool liveMode)
    {
        voicesL[0].setRatio(ratioA, liveMode);
        voicesL[1].setRatio(ratioB, liveMode);
        voicesR[0].setRatio(ratioB, liveMode);
        voicesR[1].setRatio(ratioA, liveMode);
    }

    double sampleRate = 44100.0;
    PitchMode mode = OctaveAndFifth;
    std::array<DelayPitchShifter, 2> voicesL;
    std::array<DelayPitchShifter, 2> voicesR;
    OnePoleLowPass toneL, toneR;
};

class FDNReverb
{
public:
    void prepare(double sr)
    {
        sampleRate = sr;
        maxDelayMs = 320.0f;
        for (int i = 0; i < kFDNLines; ++i)
        {
            lines[static_cast<size_t>(i)].prepare(sr, maxDelayMs);
            damping[static_cast<size_t>(i)].prepare(sr);
            lfoPhase[static_cast<size_t>(i)] = 0.073f * static_cast<float>(i + 1);
        }
        reset();
    }

    void reset()
    {
        for (auto& l : lines) l.reset();
        for (auto& d : damping) d.reset();
        std::fill(lastOut.begin(), lastOut.end(), 0.0f);
    }

    void setParameters(float decaySeconds, float size01, float tone01, float mod01, bool freezeMode, int modeIndex)
    {
        size = juce::jlimit(0.0f, 1.0f, size01);
        modDepthSamples = juce::jmap(mod01, 0.0f, 1.0f, 0.0f, 9.5f);
        modRateHz = juce::jmap(mod01, 0.0f, 1.0f, 0.035f, 0.33f);
        freeze = freezeMode;

        // Slightly voice the global modes without hiding the user controls.
        float modeDecayBoost = 1.0f;
        if (modeIndex == 1) modeDecayBoost = 1.12f;      // Cinematic
        else if (modeIndex == 2) modeDecayBoost = 0.78f; // Live
        else if (modeIndex == 3) modeDecayBoost = 1.05f; // Dark Ark
        else if (modeIndex == 4) modeDecayBoost = 1.55f; // Infinite

        decay = juce::jlimit(0.4f, 80.0f, decaySeconds * modeDecayBoost);

        const float dampingCutoff = juce::jmap(tone01, 0.0f, 1.0f, 2600.0f, 15500.0f);
        for (auto& d : damping)
            d.setCutoff(dampingCutoff);

        updateDelaySamples();
        updateFeedbackGain();
    }

    std::pair<float, float> process(float inL, float inR) noexcept
    {
        std::array<float, kFDNLines> y {};
        for (int i = 0; i < kFDNLines; ++i)
        {
            const float mod = std::sin(juce::MathConstants<float>::twoPi * lfoPhase[static_cast<size_t>(i)]) * modDepthSamples;
            lfoPhase[static_cast<size_t>(i)] += (modRateHz * (0.73f + 0.09f * i)) / static_cast<float>(sampleRate);
            if (lfoPhase[static_cast<size_t>(i)] >= 1.0f)
                lfoPhase[static_cast<size_t>(i)] -= 1.0f;

            y[static_cast<size_t>(i)] = lines[static_cast<size_t>(i)].read(delaySamples[static_cast<size_t>(i)] + mod);
        }

        // Householder-style feedback matrix: dense, cheap, stable.
        float sum = 0.0f;
        for (auto value : y)
            sum += value;
        const float mean2 = (2.0f / static_cast<float>(kFDNLines)) * sum;

        for (int i = 0; i < kFDNLines; ++i)
        {
            float matrixOut = mean2 - y[static_cast<size_t>(i)];
            matrixOut = damping[static_cast<size_t>(i)].process(matrixOut);

            const float inputSpread = inputGain(i, inL, inR);
            const float fb = freeze ? 0.9975f : feedbackGain[static_cast<size_t>(i)];
            const float write = (freeze ? 0.0f : inputSpread) + fb * matrixOut;
            lines[static_cast<size_t>(i)].push(softLimit(write));
        }

        lastOut = y;

        // Decorrelated stereo output taps.
        float outL = 0.0f, outR = 0.0f;
        static constexpr std::array<float, kFDNLines> wL { 0.35f, -0.25f, 0.31f, -0.20f, 0.24f, -0.29f, 0.21f, -0.22f };
        static constexpr std::array<float, kFDNLines> wR { -0.19f, 0.33f, -0.23f, 0.30f, -0.27f, 0.21f, -0.31f, 0.24f };
        for (int i = 0; i < kFDNLines; ++i)
        {
            outL += y[static_cast<size_t>(i)] * wL[static_cast<size_t>(i)];
            outR += y[static_cast<size_t>(i)] * wR[static_cast<size_t>(i)];
        }

        return { outL * 0.75f, outR * 0.75f };
    }

private:
    float inputGain(int line, float l, float r) const noexcept
    {
        static constexpr std::array<float, kFDNLines> lW { 0.41f, -0.23f, 0.36f, -0.28f, 0.18f, -0.35f, 0.30f, -0.21f };
        static constexpr std::array<float, kFDNLines> rW { -0.29f, 0.38f, -0.24f, 0.33f, -0.37f, 0.22f, -0.19f, 0.31f };
        return l * lW[static_cast<size_t>(line)] + r * rW[static_cast<size_t>(line)];
    }

    void updateDelaySamples()
    {
        static constexpr std::array<float, kFDNLines> baseMs { 29.7f, 37.1f, 41.9f, 53.3f, 61.7f, 71.9f, 83.3f, 97.1f };
        const float sizeScale = juce::jmap(size, 0.38f, 2.55f);
        for (int i = 0; i < kFDNLines; ++i)
        {
            delaySamples[static_cast<size_t>(i)] = juce::jlimit(
                80.0f,
                static_cast<float>(sampleRate * maxDelayMs * 0.001f - 16.0),
                baseMs[static_cast<size_t>(i)] * sizeScale * static_cast<float>(sampleRate) * 0.001f);
        }
    }

    void updateFeedbackGain()
    {
        for (int i = 0; i < kFDNLines; ++i)
        {
            const float delaySeconds = delaySamples[static_cast<size_t>(i)] / static_cast<float>(sampleRate);
            // RT60 approximation: gain for -60 dB after decay seconds.
            float g = std::pow(10.0f, -3.0f * delaySeconds / juce::jmax(0.4f, decay));
            feedbackGain[static_cast<size_t>(i)] = juce::jlimit(0.15f, 0.985f, g);
        }
    }

    double sampleRate = 44100.0;
    float maxDelayMs = 320.0f;
    float size = 0.7f;
    float decay = 6.0f;
    float modDepthSamples = 2.0f;
    float modRateHz = 0.09f;
    bool freeze = false;

    std::array<FractionalDelayLine, kFDNLines> lines;
    std::array<OnePoleLowPass, kFDNLines> damping;
    std::array<float, kFDNLines> delaySamples {};
    std::array<float, kFDNLines> feedbackGain {};
    std::array<float, kFDNLines> lfoPhase {};
    std::array<float, kFDNLines> lastOut {};
};

class Ducker
{
public:
    void prepare(double sr)
    {
        sampleRate = sr;
        reset();
        setAmount(0.25f);
    }

    void reset() noexcept { env = 0.0f; }

    void setAmount(float amount01) noexcept
    {
        amount = juce::jlimit(0.0f, 1.0f, amount01);
        attackCoeff = std::exp(-1.0f / (0.010f * static_cast<float>(sampleRate)));
        releaseCoeff = std::exp(-1.0f / (0.240f * static_cast<float>(sampleRate)));
    }

    std::pair<float, float> process(float wetL, float wetR, float dryL, float dryR) noexcept
    {
        const float input = juce::jlimit(0.0f, 1.0f, std::abs(dryL) * 0.5f + std::abs(dryR) * 0.5f);
        const float coeff = input > env ? attackCoeff : releaseCoeff;
        env = coeff * env + (1.0f - coeff) * input;

        const float reduction = amount * juce::jlimit(0.0f, 0.85f, env * 2.2f);
        const float gain = 1.0f - reduction;
        return { wetL * gain, wetR * gain };
    }

private:
    double sampleRate = 44100.0;
    float amount = 0.25f;
    float env = 0.0f;
    float attackCoeff = 0.99f;
    float releaseCoeff = 0.999f;
};

inline std::pair<float, float> applyWidth(float l, float r, float widthPercent) noexcept
{
    const float width = juce::jlimit(0.0f, 1.5f, widthPercent / 100.0f);
    const float mid = 0.5f * (l + r);
    const float side = 0.5f * (l - r) * width;
    return { mid + side, mid - side };
}

} // namespace ark
