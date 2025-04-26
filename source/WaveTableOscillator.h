//
// Created by Tyler Teuber on 4/25/25.
//

#pragma once
#include <juce_dsp/juce_dsp.h>

enum class WaveformType {
    Sine,
    Saw,
    Square,
    Triangle
};

class WavetableOscillator
{
public:
    WavetableOscillator()
    {
        // Initialize oversampling (2x, stereo, IIR filter for efficiency)
        oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
            2, // Stereo
            1, // 2x oversampling
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
            true, // High-quality upsampling
            false // Normal-quality downsampling
        );

        // Prepare wavetables
        prepareWavetables();
    }

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        oversampler->initProcessing (spec.maximumBlockSize);
        reset();
    }

    void setFrequency (float freq)
    {
        frequency = juce::jlimit<float> (0.0f, sampleRate * 0.5f, freq);
    }

    void setWaveform (WaveformType type)
    {
        waveform = type;
    }

    void setPulseWidth (float width)
    {
        // Clamp pulse width to avoid degenerate cases (0% or 100%)
        pulseWidth = juce::jlimit (0.05f, 0.95f, width);
    }

    void reset()
    {
        phase = 0.0f;
        oversampler->reset();
    }

    float getLatency() const
    {
        return oversampler->getLatencyInSamples();
    }

    void processBlock (juce::AudioBuffer<float>& buffer)
    {
        juce::dsp::AudioBlock<float> block (buffer);
        auto oversampledBlock = oversampler->processSamplesUp (block);

        // Calculate phase increment at oversampled rate
        float phaseIncrement = frequency / (sampleRate * oversampler->getOversamplingFactor());

        // Process each sample in the oversampled block
        for (int sample = 0; sample < oversampledBlock.getNumSamples(); ++sample)
        {
            float output = generateSample (phase);

            // Write to all channels
            for (int channel = 0; channel < oversampledBlock.getNumChannels(); ++channel)
            {
                oversampledBlock.setSample (channel, sample, output);
            }

            // Update phase
            phase = std::fmod (phase + phaseIncrement, 1.0f);
        }

        // Downsample back to original rate
        oversampler->processSamplesDown (block);
    }

private:
    void prepareWavetables()
    {
        const int tableSize = 1024;
        const int numTables = 4; // Frequency ranges
        const int numPulseWidths = 5; // Pulse widths: 10%, 20%, 30%, 40%, 50%
        pulseWidthValues = { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f };

        // Initialize wavetables
        sawTables.resize (numTables);
        squareTables.resize (numTables);
        triangleTables.resize (numTables);

        // Define frequency ranges (Hz)
        std::vector<float> maxFundamentals = { 500.0f, 2000.0f, 8000.0f, 16000.0f };

        for (int tableIndex = 0; tableIndex < numTables; ++tableIndex)
        {
            sawTables[tableIndex].setSize (1, tableSize);
            triangleTables[tableIndex].setSize (1, tableSize);
            squareTables[tableIndex].setSize (numPulseWidths, tableSize); // Multiple pulse widths

            float* sawSamples = sawTables[tableIndex].getWritePointer (0);
            float* triangleSamples = triangleTables[tableIndex].getWritePointer (0);

            // Calculate max harmonics for this frequency range (2x oversampling)
            int maxHarmonics = std::floor ((48000.0f * 2.0f) / (2.0f * maxFundamentals[tableIndex]));
            maxHarmonics = std::max (1, maxHarmonics);

            for (int i = 0; i < tableSize; ++i)
            {
                float phase = static_cast<float> (i) / tableSize;
                float sawValue = 0.0f;
                float triangleValue = 0.0f;

                // Generate band-limited saw and triangle
                for (int k = 1; k <= maxHarmonics; ++k)
                {
                    float angle = juce::MathConstants<float>::twoPi * phase * k;
                    float kInv = 1.0f / k;

                    // Saw: sum of (-1)^(k+1) * sin(k * phase) / k
                    sawValue += std::pow (-1.0f, k + 1) * kInv * std::sin (angle);

                    // Triangle: sum of (-1)^((k-1)/2) * sin(k * phase) / k^2 for odd k
                    if (k % 2 == 1)
                    {
                        triangleValue += std::pow (-1.0f, (k - 1) / 2) * std::sin (angle) / (k * k);
                    }
                }

                sawSamples[i] = (2.0f / juce::MathConstants<float>::pi) * sawValue;
                triangleSamples[i] = (8.0f / (juce::MathConstants<float>::pi * juce::MathConstants<float>::pi)) * triangleValue;

                // Generate square wave for each pulse width
                for (int pwIndex = 0; pwIndex < numPulseWidths; ++pwIndex)
                {
                    float* squareSamples = squareTables[tableIndex].getWritePointer (pwIndex);
                    float tau = pulseWidthValues[pwIndex];
                    float squareValue = 0.0f;

                    for (int k = 1; k <= maxHarmonics; ++k)
                    {
                        if (k % 2 == 1)
                        { // Odd harmonics
                            float kTau = k * tau * juce::MathConstants<float>::pi;
                            float angle = juce::MathConstants<float>::twoPi * k * (phase - tau / 2.0f);
                            squareValue += (std::sin (kTau) / k) * std::cos (angle);
                        }
                    }

                    squareSamples[i] = (4.0f / juce::MathConstants<float>::pi) * squareValue;
                }
            }
        }
    }

    float generateSample (const float phase) const
    {
        if (waveform == WaveformType::Sine)
        {
            return std::sin (juce::MathConstants<float>::twoPi * phase);
        }

        // Select wavetable based on frequency
        int tableIndex;
        if (frequency < 500.0f)
        {
            tableIndex = 0; // 0–500 Hz
        }
        else if (frequency < 2000.0f)
        {
            tableIndex = 1; // 500–2000 Hz
        }
        else if (frequency < 8000.0f)
        {
            tableIndex = 2; // 2000–8000 Hz
        }
        else
        {
            tableIndex = 3; // 8000+ Hz
        }

        // Select wavetable array
        const std::vector<juce::AudioSampleBuffer>* tables = nullptr;
        switch (waveform)
        {
            case WaveformType::Saw:
                tables = &sawTables;
                break;
            case WaveformType::Square:
                tables = &squareTables;
                break;
            case WaveformType::Triangle:
                tables = &triangleTables;
                break;
            default:
                return 0.0f;
        }

        // Linear interpolation for wavetable lookup
        const juce::AudioSampleBuffer& table = (*tables)[tableIndex];
        const int tableSize = table.getNumSamples();
        float index = phase * tableSize;
        int index0 = static_cast<int> (index);
        int index1 = (index0 + 1) % tableSize;
        float frac = index - index0;

        if (waveform == WaveformType::Square)
        {
            // Interpolate between pulse width wavetables
            int pwIndex0 = 0;
            int pwIndex1 = 0;
            float pwFrac = 0.0f;

            for (size_t i = 0; i < pulseWidthValues.size() - 1; ++i)
            {
                if (pulseWidth >= pulseWidthValues[i] && pulseWidth <= pulseWidthValues[i + 1])
                {
                    pwIndex0 = i;
                    pwIndex1 = i + 1;
                    pwFrac = (pulseWidth - pulseWidthValues[i]) / (pulseWidthValues[i + 1] - pulseWidthValues[i]);
                    break;
                }
            }

            // Get samples from both pulse width wavetables
            const float* samples0 = table.getReadPointer (pwIndex0);
            const float* samples1 = table.getReadPointer (pwIndex1);

            float value0 = samples0[index0] + frac * (samples0[index1] - samples0[index0]);
            float value1 = samples1[index0] + frac * (samples1[index1] - samples1[index0]);

            // Interpolate between pulse widths
            return value0 + pwFrac * (value1 - value0);
        }
        else
        {
            // Standard wavetable lookup for saw and triangle
            const float* samples = table.getReadPointer (0);
            return samples[index0] + frac * (samples[index1] - samples[index0]);
        }
    }

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    std::vector<juce::AudioSampleBuffer> sawTables, squareTables, triangleTables;
    std::vector<float> pulseWidthValues;
    WaveformType waveform = WaveformType::Square;
    float frequency = 440.0f;
    float pulseWidth = 0.1f; // Default 50% pulse width
    float phase = 0.0f;
    double sampleRate = 48000.0;
};