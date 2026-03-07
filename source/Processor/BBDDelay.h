#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include "../Utility/AtomicHelpers.h"
#include "../Utility/Parameters.h"

class BBDDelay : public juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit BBDDelay (juce::AudioProcessorValueTreeState& p)
        : parameters (p)
    {
        parameters.addParameterListener (ParamIDs::delayOn, this);
        parameters.addParameterListener (ParamIDs::delayTime, this);
        parameters.addParameterListener (ParamIDs::delayTempoSync, this);
        parameters.addParameterListener (ParamIDs::delayNoteDivision, this);
        parameters.addParameterListener (ParamIDs::delayFeedback, this);
        parameters.addParameterListener (ParamIDs::delayMix, this);
        parameters.addParameterListener (ParamIDs::delayLowpass, this);
        parameters.addParameterListener (ParamIDs::delayHighpass, this);
        parameters.addParameterListener (ParamIDs::delaySaturation, this);
        parameters.addParameterListener (ParamIDs::delayModRate, this);
        parameters.addParameterListener (ParamIDs::delayModDepth, this);
        parameters.addParameterListener (ParamIDs::delayPingPong, this);
        parameters.addParameterListener (ParamIDs::delayDiffusion, this);
    }

    ~BBDDelay() override
    {
        parameters.removeParameterListener (ParamIDs::delayOn, this);
        parameters.removeParameterListener (ParamIDs::delayTime, this);
        parameters.removeParameterListener (ParamIDs::delayTempoSync, this);
        parameters.removeParameterListener (ParamIDs::delayNoteDivision, this);
        parameters.removeParameterListener (ParamIDs::delayFeedback, this);
        parameters.removeParameterListener (ParamIDs::delayMix, this);
        parameters.removeParameterListener (ParamIDs::delayLowpass, this);
        parameters.removeParameterListener (ParamIDs::delayHighpass, this);
        parameters.removeParameterListener (ParamIDs::delaySaturation, this);
        parameters.removeParameterListener (ParamIDs::delayModRate, this);
        parameters.removeParameterListener (ParamIDs::delayModDepth, this);
        parameters.removeParameterListener (ParamIDs::delayPingPong, this);
        parameters.removeParameterListener (ParamIDs::delayDiffusion, this);
    }

    void parameterChanged (const juce::String& parameterID, float newValue) override
    {
        if (parameterID == ParamIDs::delayOn)               AtomicHelpers::paramStore (enabled, newValue >= 0.5f ? 1 : 0);
        else if (parameterID == ParamIDs::delayTime)     AtomicHelpers::paramStore (delayTimeMs, newValue);
        else if (parameterID == ParamIDs::delayTempoSync) AtomicHelpers::paramStore (tempoSync, newValue >= 0.5f ? 1.0f : 0.0f);
        else if (parameterID == ParamIDs::delayNoteDivision) AtomicHelpers::paramStore (noteDivision, newValue);
        else if (parameterID == ParamIDs::delayFeedback)  AtomicHelpers::paramStore (feedback, newValue);
        else if (parameterID == ParamIDs::delayMix)       AtomicHelpers::paramStore (mix, newValue);
        else if (parameterID == ParamIDs::delayLowpass)   AtomicHelpers::paramStore (lowpassFreq, newValue);
        else if (parameterID == ParamIDs::delayHighpass)  AtomicHelpers::paramStore (highpassFreq, newValue);
        else if (parameterID == ParamIDs::delaySaturation) AtomicHelpers::paramStore (saturation, newValue);
        else if (parameterID == ParamIDs::delayModRate)   AtomicHelpers::paramStore (modRate, newValue);
        else if (parameterID == ParamIDs::delayModDepth)  AtomicHelpers::paramStore (modDepth, newValue);
        else if (parameterID == ParamIDs::delayPingPong)  AtomicHelpers::paramStore (pingPong, newValue >= 0.5f ? 1.0f : 0.0f);
        else if (parameterID == ParamIDs::delayDiffusion) AtomicHelpers::paramStore (diffusionAmt, newValue);
    }

    void setTempoInfo (double bpm, bool available)
    {
        currentBpm = bpm;
        hostTempoAvailable = available;
    }

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;

        // Max delay: 2.05 seconds
        int maxDelaySamples = static_cast<int> (sampleRate * 2.05) + 1;
        for (int ch = 0; ch < 2; ++ch)
        {
            delayLines[ch].prepare (spec);
            delayLines[ch].setMaximumDelayInSamples (maxDelaySamples);

            lpFilters[ch].prepare (spec);
            lpFilters[ch].setType (juce::dsp::FirstOrderTPTFilterType::lowpass);
            lpFilters[ch].setCutoffFrequency (4000.0f);

            hpFilters[ch].prepare (spec);
            hpFilters[ch].setType (juce::dsp::FirstOrderTPTFilterType::highpass);
            hpFilters[ch].setCutoffFrequency (80.0f);

            dcBlockFilters[ch].prepare (spec);
            dcBlockFilters[ch].setType (juce::dsp::FirstOrderTPTFilterType::highpass);
            dcBlockFilters[ch].setCutoffFrequency (5.0f);
        }

        // Input diffusion allpass filters: 4 per channel
        constexpr float diffDelaysMs[4] = { 1.3f, 2.1f, 3.5f, 4.7f };
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 4; ++i)
            {
                int delaySamples = static_cast<int> (std::round (diffDelaysMs[i] / 1000.0f * static_cast<float> (sampleRate)));
                diffusers[ch][i].prepare (delaySamples);
            }

        // Initialize smoothing
        smoothedDelaySamples = 0.0f;
        smoothedFeedback = 0.4f;
        lfoPhaseL = 0.0f;
        lfoPhaseR = static_cast<float> (M_PI) * 0.5f; // 90-degree offset
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        juce::ScopedNoDenormals noDenormals;

        if (buffer.getNumChannels() < 2) return;

        if (AtomicHelpers::paramLoad (enabled) == 0)
            return;

        // Load atomic parameters once per block
        const float localDelayMs    = AtomicHelpers::paramLoad (delayTimeMs);
        const bool  localTempoSync  = AtomicHelpers::paramLoad (tempoSync) >= 0.5f;
        const int   localDivision   = static_cast<int> (AtomicHelpers::paramLoad (noteDivision));
        const float localFeedback   = AtomicHelpers::paramLoad (feedback);
        const float localMix        = AtomicHelpers::paramLoad (mix);
        const float localLowpass    = AtomicHelpers::paramLoad (lowpassFreq);
        const float localHighpass   = AtomicHelpers::paramLoad (highpassFreq);
        const float localSaturation = AtomicHelpers::paramLoad (saturation);
        const float localModRate    = AtomicHelpers::paramLoad (modRate);
        const float localModDepth   = AtomicHelpers::paramLoad (modDepth);
        const bool  localPingPong   = AtomicHelpers::paramLoad (pingPong) >= 0.5f;
        const float localDiffusion  = AtomicHelpers::paramLoad (diffusionAmt);

        // Compute target delay time in ms
        float targetDelayMs = localDelayMs;
        if (localTempoSync)
        {
            double bpm = hostTempoAvailable ? currentBpm : static_cast<double> (*parameters.getRawParameterValue (ParamIDs::manualBpm));
            if (bpm < 20.0) bpm = 120.0;
            double beatMs = 60000.0 / bpm;
            targetDelayMs = static_cast<float> (beatMs * getBeatMultiplier (localDivision));
            targetDelayMs = juce::jlimit (10.0f, 2000.0f, targetDelayMs);
        }

        float targetDelaySamples = targetDelayMs / 1000.0f * static_cast<float> (sampleRate);

        // Update filter cutoffs
        float clampedLP = std::min (localLowpass, static_cast<float> (sampleRate * 0.49));
        float clampedHP = std::min (localHighpass, static_cast<float> (sampleRate * 0.49));
        for (int ch = 0; ch < 2; ++ch)
        {
            lpFilters[ch].setCutoffFrequency (clampedLP);
            hpFilters[ch].setCutoffFrequency (clampedHP);
        }

        // Saturation drive
        const float drive = 1.0f + localSaturation * 3.0f;
        const float invDrive = 1.0f / drive;

        // LFO increment per sample
        const float lfoInc = 2.0f * static_cast<float> (M_PI) * localModRate / static_cast<float> (sampleRate);

        // Max modulation depth: 3ms in samples
        const float maxModSamples = 0.003f * static_cast<float> (sampleRate);
        const float modDepthSamples = localModDepth * maxModSamples;

        // Smoothing coefficients (one-pole)
        // ~5ms time constant for delay time
        const float delaySmooth = 1.0f - std::exp (-1.0f / (0.005f * static_cast<float> (sampleRate)));
        // ~10ms time constant for feedback
        const float fbSmooth = 1.0f - std::exp (-1.0f / (0.01f * static_cast<float> (sampleRate)));

        // Diffusion gain
        const float diffGain = localDiffusion * 0.7f;
        const bool useDiffusion = localDiffusion > 0.001f;

        auto* leftChannel  = buffer.getWritePointer (0);
        auto* rightChannel = buffer.getWritePointer (1);
        const int numSamples = buffer.getNumSamples();

        for (int n = 0; n < numSamples; ++n)
        {
            // Smooth delay time and feedback
            smoothedDelaySamples += delaySmooth * (targetDelaySamples - smoothedDelaySamples);
            smoothedFeedback += fbSmooth * (localFeedback - smoothedFeedback);

            // Compute LFO modulation for L and R (sine, 90-degree offset)
            float lfoModL = std::sin (lfoPhaseL) * modDepthSamples;
            float lfoModR = std::sin (lfoPhaseR) * modDepthSamples;
            lfoPhaseL += lfoInc;
            lfoPhaseR += lfoInc;
            if (lfoPhaseL >= 2.0f * static_cast<float> (M_PI))
                lfoPhaseL -= 2.0f * static_cast<float> (M_PI);
            if (lfoPhaseR >= 2.0f * static_cast<float> (M_PI))
                lfoPhaseR -= 2.0f * static_cast<float> (M_PI);

            // Read from delay lines at modulated positions
            float readDelayL = std::max (smoothedDelaySamples + lfoModL, 1.0f);
            float readDelayR = std::max (smoothedDelaySamples + lfoModR, 1.0f);

            delayLines[0].setDelay (readDelayL);
            delayLines[1].setDelay (readDelayR);

            float wetL = delayLines[0].popSample (0);
            float wetR = delayLines[1].popSample (0);

            // Feedback path: LP -> HP -> tanh saturation -> multiply feedback gain
            float fbL = lpFilters[0].processSample (0, wetL);
            fbL = hpFilters[0].processSample (0, fbL);
            fbL = std::tanh (fbL * drive) * invDrive;
            fbL *= smoothedFeedback;

            float fbR = lpFilters[1].processSample (0, wetR);
            fbR = hpFilters[1].processSample (0, fbR);
            fbR = std::tanh (fbR * drive) * invDrive;
            fbR *= smoothedFeedback;

            // Ping-pong: swap L/R feedback
            if (localPingPong)
                std::swap (fbL, fbR);

            // Input signals
            float inputL = leftChannel[n];
            float inputR = rightChannel[n];

            // Diffusion: run input through 4 allpass cascades
            if (useDiffusion)
            {
                for (int i = 0; i < 4; ++i)
                {
                    inputL = diffusers[0][i].processSample (inputL, diffGain);
                    inputR = diffusers[1][i].processSample (inputR, diffGain);
                }
            }

            // Write (input + feedback) to delay lines
            delayLines[0].pushSample (0, inputL + fbL);
            delayLines[1].pushSample (0, inputR + fbR);

            // DC block wet signal
            wetL = dcBlockFilters[0].processSample (0, wetL);
            wetR = dcBlockFilters[1].processSample (0, wetR);

            // Wet/dry mix
            leftChannel[n]  = leftChannel[n]  * (1.0f - localMix) + wetL * localMix;
            rightChannel[n] = rightChannel[n] * (1.0f - localMix) + wetR * localMix;
        }
    }

private:
    juce::AudioProcessorValueTreeState& parameters;

    std::atomic<int> enabled { 0 };
    std::atomic<float> delayTimeMs { 375.0f };
    std::atomic<float> tempoSync { 0.0f };
    std::atomic<float> noteDivision { 4.0f };
    std::atomic<float> feedback { 0.4f };
    std::atomic<float> mix { 0.3f };
    std::atomic<float> lowpassFreq { 4000.0f };
    std::atomic<float> highpassFreq { 80.0f };
    std::atomic<float> saturation { 0.3f };
    std::atomic<float> modRate { 0.7f };
    std::atomic<float> modDepth { 0.15f };
    std::atomic<float> pingPong { 0.0f };
    std::atomic<float> diffusionAmt { 0.0f };

    double sampleRate = 44100.0;
    double currentBpm = 120.0;
    bool hostTempoAvailable = false;

    // Smoothing state
    float smoothedDelaySamples = 0.0f;
    float smoothedFeedback = 0.4f;

    // LFO state
    float lfoPhaseL = 0.0f;
    float lfoPhaseR = 0.0f;

    // Delay lines (Lagrange 3rd order for smooth modulated reads)
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLines[2];

    // Feedback filters
    juce::dsp::FirstOrderTPTFilter<float> lpFilters[2];
    juce::dsp::FirstOrderTPTFilter<float> hpFilters[2];

    // DC blocking
    juce::dsp::FirstOrderTPTFilter<float> dcBlockFilters[2];

    //==========================================================================
    // DiffusionAllpass — lightweight allpass for input diffusion
    //==========================================================================
    struct DiffusionAllpass
    {
        void prepare (int delaySamples)
        {
            baseDelay = delaySamples;
            buffer.assign (static_cast<size_t> (delaySamples + 1), 0.0f);
            bufferSize = delaySamples + 1;
            writePos = 0;
        }

        float processSample (float input, float gain = 0.6f)
        {
            int readPos = writePos - baseDelay;
            if (readPos < 0) readPos += bufferSize;

            float delayed = buffer[static_cast<size_t> (readPos)];
            float temp = input + gain * delayed;
            float output = delayed - gain * temp;

            buffer[static_cast<size_t> (writePos)] = temp;
            writePos = (writePos + 1) % bufferSize;
            return output;
        }

        std::vector<float> buffer;
        int bufferSize = 0;
        int baseDelay = 0;
        int writePos = 0;
    };

    DiffusionAllpass diffusers[2][4];

    //==========================================================================
    // Beat multiplier table for tempo sync
    //==========================================================================
    static double getBeatMultiplier (int divisionIndex)
    {
        // "1/1", "1/2", "1/2T", "1/2D", "1/4", "1/4T", "1/4D",
        // "1/8", "1/8T", "1/8D", "1/16", "1/16T", "1/16D"
        constexpr double multipliers[] = {
            4.0,            // 1/1 = 4 beats
            2.0,            // 1/2 = 2 beats
            4.0 / 3.0,      // 1/2T = triplet
            3.0,            // 1/2D = dotted
            1.0,            // 1/4 = 1 beat
            2.0 / 3.0,      // 1/4T = triplet
            1.5,            // 1/4D = dotted
            0.5,            // 1/8
            1.0 / 3.0,      // 1/8T
            0.75,           // 1/8D
            0.25,           // 1/16
            1.0 / 6.0,      // 1/16T
            0.375           // 1/16D
        };
        if (divisionIndex >= 0 && divisionIndex < 13)
            return multipliers[divisionIndex];
        return 1.0;
    }
};
