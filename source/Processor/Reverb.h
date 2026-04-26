#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include "../Utility/AtomicHelpers.h"
#include "../Utility/Parameters.h"

class Reverb : public juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit Reverb (juce::AudioProcessorValueTreeState& p)
        : parameters (p)
    {
        AtomicHelpers::paramStore (enabled, *p.getRawParameterValue (ParamIDs::reverbOn) >= 0.5f ? 1 : 0);
        parameters.addParameterListener (ParamIDs::reverbOn, this);
        parameters.addParameterListener (ParamIDs::reverbDecay, this);
        parameters.addParameterListener (ParamIDs::reverbSize, this);
        parameters.addParameterListener (ParamIDs::reverbDamping, this);
        parameters.addParameterListener (ParamIDs::reverbBassMult, this);
        parameters.addParameterListener (ParamIDs::reverbModRate, this);
        parameters.addParameterListener (ParamIDs::reverbModDepth, this);
        parameters.addParameterListener (ParamIDs::reverbDiffusion, this);
        parameters.addParameterListener (ParamIDs::reverbPreDelay, this);
        parameters.addParameterListener (ParamIDs::reverbWidth, this);
        parameters.addParameterListener (ParamIDs::reverbLevel, this);
    }

    ~Reverb() override
    {
        parameters.removeParameterListener (ParamIDs::reverbOn, this);
        parameters.removeParameterListener (ParamIDs::reverbDecay, this);
        parameters.removeParameterListener (ParamIDs::reverbSize, this);
        parameters.removeParameterListener (ParamIDs::reverbDamping, this);
        parameters.removeParameterListener (ParamIDs::reverbBassMult, this);
        parameters.removeParameterListener (ParamIDs::reverbModRate, this);
        parameters.removeParameterListener (ParamIDs::reverbModDepth, this);
        parameters.removeParameterListener (ParamIDs::reverbDiffusion, this);
        parameters.removeParameterListener (ParamIDs::reverbPreDelay, this);
        parameters.removeParameterListener (ParamIDs::reverbWidth, this);
        parameters.removeParameterListener (ParamIDs::reverbLevel, this);
    }

    void parameterChanged (const juce::String& parameterID, float newValue) override
    {
        if (parameterID == ParamIDs::reverbOn)             AtomicHelpers::paramStore (enabled, newValue >= 0.5f ? 1 : 0);
        else if (parameterID == ParamIDs::reverbDecay) AtomicHelpers::paramStore (decay, newValue);
        else if (parameterID == ParamIDs::reverbSize)    AtomicHelpers::paramStore (size, newValue);
        else if (parameterID == ParamIDs::reverbDamping) AtomicHelpers::paramStore (dampingFreq, newValue);
        else if (parameterID == ParamIDs::reverbBassMult) AtomicHelpers::paramStore (bassMult, newValue);
        else if (parameterID == ParamIDs::reverbModRate)  AtomicHelpers::paramStore (modRate, newValue);
        else if (parameterID == ParamIDs::reverbModDepth) AtomicHelpers::paramStore (modDepth, newValue);
        else if (parameterID == ParamIDs::reverbDiffusion) AtomicHelpers::paramStore (diffusion, newValue);
        else if (parameterID == ParamIDs::reverbPreDelay)  AtomicHelpers::paramStore (preDelayMs, newValue);
        else if (parameterID == ParamIDs::reverbWidth)     AtomicHelpers::paramStore (width, newValue);
        else if (parameterID == ParamIDs::reverbLevel)       AtomicHelpers::paramStore (level, newValue);
    }

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;

        // Pre-delay: max 300 ms
        preDelay.prepare (spec);
        preDelay.setMaximumDelayInSamples (static_cast<int> (sampleRate * 0.35));
        preDelay.setDelay (0.0f);

        // Input diffusion: 4 cascaded allpass stages
        constexpr float inputDiffDelaysMs[4] = { 1.5f, 2.3f, 3.7f, 5.1f };
        for (int i = 0; i < 4; ++i)
        {
            int delaySamples = static_cast<int> (std::round (inputDiffDelaysMs[i] / 1000.0f * sampleRate));
            inputDiffusers[i].prepare (delaySamples, sampleRate, 0.7f + 0.15f * i, 1.5f);
        }
        inputDiffDamper.prepare (spec);
        inputDiffDamper.setType (juce::dsp::FirstOrderTPTFilterType::lowpass);
        inputDiffDamper.setCutoffFrequency (10000.0f);

        // Main FDN: 16 delay lines with prime ms values
        constexpr float primeDelaysMs[16] = {
            19.0f, 23.0f, 29.0f, 31.0f, 37.0f, 41.0f, 43.0f, 47.0f,
            53.0f, 59.0f, 61.0f, 67.0f, 71.0f, 73.0f, 79.0f, 83.0f
        };
        for (int i = 0; i < 16; ++i)
        {
            baseDelayMs[i] = primeDelaysMs[i];
            // Max delay: baseDelay * 2.2 (size) + modulation headroom + 4 samples
            int maxSamples = static_cast<int> ((primeDelaysMs[i] * 2.2f / 1000.0f) * sampleRate) + 100;
            fdnLines[i].prepare (spec);
            fdnLines[i].setMaximumDelayInSamples (maxSamples);

            // Damping lowpass (TPT)
            dampingFilters[i].prepare (spec);
            dampingFilters[i].setType (juce::dsp::FirstOrderTPTFilterType::lowpass);
            dampingFilters[i].setCutoffFrequency (6000.0f);

            // Crossover state for frequency-dependent decay (one-pole at 250 Hz)
            crossoverCoeff[i] = 0.0f;
            crossoverLowState[i] = 0.0f;

            // LFO state — golden ratio phase offsets
            lfoPhase[i] = static_cast<float> (i) * 0.6180339887f * 2.0f * static_cast<float> (M_PI);
            while (lfoPhase[i] >= 2.0f * static_cast<float> (M_PI))
                lfoPhase[i] -= 2.0f * static_cast<float> (M_PI);
        }

        // Update crossover coefficient for 250 Hz
        float w = 2.0f * static_cast<float> (M_PI) * 250.0f / static_cast<float> (sampleRate);
        float g = w / (1.0f + w);
        for (int i = 0; i < 16; ++i)
            crossoverCoeff[i] = g;

        // Output diffusion: 2 allpass per channel
        constexpr float outDiffDelaysMs[2] = { 3.07f, 4.51f };
        for (int i = 0; i < 2; ++i)
        {
            int ds = static_cast<int> (std::round (outDiffDelaysMs[i] / 1000.0f * sampleRate));
            outputDiffusersL[i].prepare (ds, sampleRate, 0.6f + 0.25f * i, 1.5f);
            outputDiffusersR[i].prepare (ds, sampleRate, 0.65f + 0.3f * i, 1.5f);
        }

        // DC blocking highpass (10 Hz)
        dcBlockL.prepare (spec);
        dcBlockR.prepare (spec);
        dcBlockL.setType (juce::dsp::FirstOrderTPTFilterType::highpass);
        dcBlockR.setType (juce::dsp::FirstOrderTPTFilterType::highpass);
        dcBlockL.setCutoffFrequency (10.0f);
        dcBlockR.setCutoffFrequency (10.0f);
    }

    void reset()
    {
        preDelay.reset();
        for (int i = 0; i < 4; ++i)
            inputDiffusers[i].reset();
        inputDiffDamper.reset();
        for (int i = 0; i < 16; ++i)
        {
            fdnLines[i].reset();
            dampingFilters[i].reset();
            crossoverLowState[i] = 0.0f;
            lfoPhase[i] = static_cast<float> (i) * 0.6180339887f * 2.0f * static_cast<float> (M_PI);
            while (lfoPhase[i] >= 2.0f * static_cast<float> (M_PI))
                lfoPhase[i] -= 2.0f * static_cast<float> (M_PI);
        }
        for (int i = 0; i < 2; ++i)
        {
            outputDiffusersL[i].reset();
            outputDiffusersR[i].reset();
        }
        dcBlockL.reset();
        dcBlockR.reset();
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        juce::ScopedNoDenormals noDenormals;

        if (buffer.getNumChannels() < 2) return;

        if (AtomicHelpers::paramLoad (enabled) == 0)
            return;

        // Load atomic parameters once per block
        const float localDecay      = AtomicHelpers::paramLoad (decay);
        const float localSize       = AtomicHelpers::paramLoad (size);
        const float localDamping    = AtomicHelpers::paramLoad (dampingFreq);
        const float localBassMult   = AtomicHelpers::paramLoad (bassMult);
        const float localModRate    = AtomicHelpers::paramLoad (modRate);
        const float localModDepth   = AtomicHelpers::paramLoad (modDepth);
        const float localDiffusion  = AtomicHelpers::paramLoad (diffusion);
        const float localPreDelay   = AtomicHelpers::paramLoad (preDelayMs);
        const float localWidth      = AtomicHelpers::paramLoad (width);
        const float localLevel      = AtomicHelpers::paramLoad (level);

        auto* leftChannel  = buffer.getWritePointer (0);
        auto* rightChannel = buffer.getWritePointer (1);
        const int numSamples = buffer.getNumSamples();

        // Update pre-delay
        preDelay.setDelay (static_cast<float> (localPreDelay / 1000.0f * sampleRate));

        // Clamp damping to Nyquist
        float clampedDamping = std::min (localDamping, static_cast<float> (sampleRate * 0.49));
        for (int i = 0; i < 16; ++i)
            dampingFilters[i].setCutoffFrequency (clampedDamping);

        // Precompute per-line feedback coefficients (bass and high separately)
        std::array<float, 16> fbHigh {};
        std::array<float, 16> fbLow {};
        for (int i = 0; i < 16; ++i)
        {
            float delayTimeSec = (baseDelayMs[i] * localSize) / 1000.0f;
            fbHigh[i] = std::pow (10.0f, -3.0f * delayTimeSec / std::max (localDecay, 0.01f));
            fbLow[i]  = std::pow (10.0f, -3.0f * delayTimeSec / std::max (localDecay * localBassMult, 0.01f));
        }

        // Precompute LFO increments per line
        std::array<float, 16> lfoInc {};
        for (int i = 0; i < 16; ++i)
        {
            float freq = localModRate * (0.7f + 0.4f * static_cast<float> (i) / 15.0f);
            lfoInc[i] = 2.0f * static_cast<float> (M_PI) * freq / static_cast<float> (sampleRate);
        }

        const float inputScale = 1.0f / 4.0f; // 1/sqrt(16) = 0.25
        const float diffGain = localDiffusion * 0.75f;

        for (int n = 0; n < numSamples; ++n)
        {
            // Mono sum input
            float input = (leftChannel[n] + rightChannel[n]) * 0.5f;

            // 1. Pre-delay
            preDelay.pushSample (0, input);
            float preDelayed = preDelay.popSample (0);

            // 2. Input diffusion: 4 cascaded allpass
            float diffused = preDelayed;
            for (int i = 0; i < 4; ++i)
                diffused = inputDiffusers[i].processSample (diffused, diffGain);
            diffused = inputDiffDamper.processSample (0, diffused);

            // 3. Read from FDN delay lines with modulated positions
            std::array<float, 16> delayReads {};
            for (int i = 0; i < 16; ++i)
            {
                float baseDelaySamp = (baseDelayMs[i] * localSize / 1000.0f) * static_cast<float> (sampleRate);
                float modDepthSamp = localModDepth * 0.01f * baseDelaySamp;
                float mod = std::sin (lfoPhase[i]) * modDepthSamp;
                lfoPhase[i] += lfoInc[i];
                if (lfoPhase[i] >= 2.0f * static_cast<float> (M_PI))
                    lfoPhase[i] -= 2.0f * static_cast<float> (M_PI);

                float readDelay = baseDelaySamp + mod;
                readDelay = std::max (readDelay, 1.0f);
                fdnLines[i].setDelay (readDelay);
                delayReads[i] = fdnLines[i].popSample (0);
            }

            // 4. In-place Walsh-Hadamard 16x16 transform
            std::array<float, 16> mixed = delayReads;
            hadamardInPlace (mixed.data(), 16);

            // 5. Per-line: crossover split, freq-dependent feedback, damping, soft-clip, inject input
            for (int i = 0; i < 16; ++i)
            {
                float signal = mixed[i];

                // One-pole crossover at 250 Hz
                float low = crossoverLowState[i] + crossoverCoeff[i] * (signal - crossoverLowState[i]);
                crossoverLowState[i] = low;
                float high = signal - low;

                // Frequency-dependent feedback
                float fb = low * fbLow[i] + high * fbHigh[i];

                // HF damping lowpass
                fb = dampingFilters[i].processSample (0, fb);

                // Soft-clip to prevent blowup
                fb = std::tanh (fb);

                // Inject input (scaled to prevent 16x amplification)
                float feedInput = diffused * inputScale + fb;

                fdnLines[i].pushSample (0, feedInput);
            }

            // 6. Stereo output taps: 8 per channel, alternating signs
            float outL = 0.0f, outR = 0.0f;
            for (int i = 0; i < 8; ++i)
            {
                float sign = (i % 2 == 0) ? 1.0f : -1.0f;
                outL += sign * delayReads[i];
                outR += sign * delayReads[i + 8];
            }
            outL *= 0.25f; // 1/sqrt(16) scaling
            outR *= 0.25f;

            // 7. DC blocking
            outL = dcBlockL.processSample (0, outL);
            outR = dcBlockR.processSample (0, outR);

            // 8. Output diffusion (2 allpass per channel)
            for (int i = 0; i < 2; ++i)
            {
                outL = outputDiffusersL[i].processSample (outL);
                outR = outputDiffusersR[i].processSample (outR);
            }

            // 9. Mid/side stereo width
            float mid  = (outL + outR) * 0.5f;
            float side = (outL - outR) * 0.5f;
            side *= localWidth;
            outL = mid + side;
            outR = mid - side;

            // 10. Add wet signal at level (dry stays at 100%)
            leftChannel[n]  = leftChannel[n] + outL * localLevel;
            rightChannel[n] = rightChannel[n] + outR * localLevel;
        }
    }

private:
    juce::AudioProcessorValueTreeState& parameters;

    std::atomic<int> enabled { 0 };
    std::atomic<float> decay { 0.5f };
    std::atomic<float> size { 0.5f };
    std::atomic<float> dampingFreq { 6000.0f };
    std::atomic<float> bassMult { 1.0f };
    std::atomic<float> modRate { 0.5f };
    std::atomic<float> modDepth { 0.0f };
    std::atomic<float> diffusion { 0.7f };
    std::atomic<float> preDelayMs { 0.0f };
    std::atomic<float> width { 1.0f };
    std::atomic<float> level { 0.0f };

    double sampleRate = 44100.0;

    //==========================================================================
    // DiffusionAllpass — lightweight allpass for input/output diffusion
    //==========================================================================
    struct DiffusionAllpass
    {
        void prepare (int delaySamples, double sr, float modRateHz = 1.0f, float modDepthVal = 1.5f)
        {
            baseDelay = delaySamples;
            modDepthSamples = modDepthVal;
            lfoInc = static_cast<float> (2.0 * M_PI * modRateHz / sr);
            lfoPhase = 0.0f;

            int maxDelay = delaySamples + static_cast<int> (modDepthVal) + 2;
            buffer.assign (static_cast<size_t> (maxDelay), 0.0f);
            bufferSize = maxDelay;
            writePos = 0;
        }

        float processSample (float input, float gain = 0.6f)
        {
            float mod = std::sin (lfoPhase) * modDepthSamples;
            lfoPhase += lfoInc;
            if (lfoPhase >= 2.0f * static_cast<float> (M_PI))
                lfoPhase -= 2.0f * static_cast<float> (M_PI);

            float readDelay = static_cast<float> (baseDelay) + mod;
            float readPosF = static_cast<float> (writePos) - readDelay;
            if (readPosF < 0.0f) readPosF += static_cast<float> (bufferSize);

            int idx0 = static_cast<int> (readPosF);
            int idx1 = (idx0 + 1) % bufferSize;
            float frac = readPosF - static_cast<float> (idx0);
            float delayed = buffer[static_cast<size_t> (idx0)] * (1.0f - frac) + buffer[static_cast<size_t> (idx1)] * frac;

            float temp = input + gain * delayed;
            float output = delayed - gain * temp;

            buffer[static_cast<size_t> (writePos)] = temp;
            writePos = (writePos + 1) % bufferSize;
            return output;
        }

        void reset()
        {
            std::fill (buffer.begin(), buffer.end(), 0.0f);
            writePos = 0;
            lfoPhase = 0.0f;
        }

        std::vector<float> buffer;
        int bufferSize = 0;
        int baseDelay = 0;
        int writePos = 0;
        float lfoPhase = 0.0f;
        float lfoInc = 0.0f;
        float modDepthSamples = 1.5f;
    };

    //==========================================================================
    // In-place Fast Walsh-Hadamard Transform, scaled by 1/sqrt(N)
    //==========================================================================
    static void hadamardInPlace (float* data, int n)
    {
        // n must be a power of 2
        for (int len = 1; len < n; len <<= 1)
        {
            for (int i = 0; i < n; i += len << 1)
            {
                for (int j = 0; j < len; ++j)
                {
                    float a = data[i + j];
                    float b = data[i + j + len];
                    data[i + j]       = a + b;
                    data[i + j + len] = a - b;
                }
            }
        }
        // Scale by 1/sqrt(N)
        float scale = 1.0f / std::sqrt (static_cast<float> (n));
        for (int i = 0; i < n; ++i)
            data[i] *= scale;
    }

    // Pre-delay
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelay;

    // Input diffusion
    DiffusionAllpass inputDiffusers[4];
    juce::dsp::FirstOrderTPTFilter<float> inputDiffDamper;

    // FDN: 16 delay lines (Lagrange3rd interpolation for modulated reads)
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> fdnLines[16];
    std::array<float, 16> baseDelayMs {};

    // Per-line damping lowpass
    juce::dsp::FirstOrderTPTFilter<float> dampingFilters[16];

    // Per-line crossover state (one-pole at 250 Hz)
    std::array<float, 16> crossoverCoeff {};
    std::array<float, 16> crossoverLowState {};

    // Per-line LFO state
    std::array<float, 16> lfoPhase {};

    // Output diffusion
    DiffusionAllpass outputDiffusersL[2];
    DiffusionAllpass outputDiffusersR[2];

    // DC blocking
    juce::dsp::FirstOrderTPTFilter<float> dcBlockL;
    juce::dsp::FirstOrderTPTFilter<float> dcBlockR;
};
