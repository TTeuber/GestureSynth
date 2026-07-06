#include <PluginProcessor.h>
#include <Processor/JuneOscillator.h>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

namespace
{
constexpr double kSampleRate = 44100.0;
constexpr int kBlockSize = 512;
constexpr int kFftOrder = 15; // 32768-point FFT
constexpr int kFftSize = 1 << kFftOrder;

// Renders kFftSize samples from a JuneDCO at the given frequency and waveform
// mix (0 = saw, 1 = pulse), after discarding a warm-up block for the
// oversampler's filters to settle. If subWaveMix >= 0 the sub-oscillator is
// rendered alone instead (0 = sine, 1/3 = triangle, 2/3 = square, 1 = saw).
std::vector<float> renderDCO (PluginProcessor& plugin, float frequency, float waveformMix,
                              float subWaveMix = -1.0f)
{
    auto& apvts = plugin.parameters;
    DynamicParameter pulseWidth (apvts.getParameter (ParamIDs::pulseWidth));
    DynamicParameter waveform (apvts.getParameter (ParamIDs::oscWaveform));
    DynamicParameter detune (apvts.getParameter (ParamIDs::oscDetune));
    DynamicParameter width (apvts.getParameter (ParamIDs::oscWidth));
    DynamicParameter subLevel (apvts.getParameter (ParamIDs::subOsc));
    DynamicParameter subWave (apvts.getParameter (ParamIDs::subOscWave));
    DynamicParameter mainLevel (apvts.getParameter (ParamIDs::mainOscLevel));

    JuneDCO dco (apvts, pulseWidth, waveform, detune, width, subLevel, subWave, mainLevel);

    // Single tone, no detune spread, full level — keeps every partial an exact
    // harmonic of the fundamental so alias energy is cleanly measurable.
    detune.setCurrentValue (0.0f);
    waveform.setCurrentValue (waveformMix);
    pulseWidth.setCurrentValue (0.5f);

    if (subWaveMix >= 0.0f)
    {
        apvts.getParameter (ParamIDs::subOn)->setValueNotifyingHost (1.0f);
        mainLevel.setCurrentValue (0.0f);
        subLevel.setCurrentValue (1.0f);
        subWave.setCurrentValue (subWaveMix);
    }
    else
    {
        mainLevel.setCurrentValue (1.0f);
        subLevel.setCurrentValue (0.0f);
    }

    dco.prepare ({ kSampleRate, static_cast<juce::uint32> (kBlockSize), 2 });
    dco.setFrequency (frequency);

    std::vector<float> samples;
    samples.reserve (kFftSize);

    juce::AudioBuffer<float> buffer (2, kBlockSize);
    const int warmupBlocks = 8;
    const int totalBlocks = warmupBlocks + (kFftSize + kBlockSize - 1) / kBlockSize;

    for (int b = 0; b < totalBlocks; ++b)
    {
        buffer.clear();
        juce::dsp::AudioBlock<float> block (buffer);
        dco.processBlock (block);

        if (b < warmupBlocks)
            continue;

        const auto* data = buffer.getReadPointer (0);
        for (int i = 0; i < kBlockSize && samples.size() < static_cast<size_t> (kFftSize); ++i)
            samples.push_back (data[i]);
    }

    return samples;
}

// Returns the level (in dB relative to the strongest harmonic) of the loudest
// spectral component that is NOT a harmonic of the fundamental — i.e. aliasing.
// The measurement stops at 20 kHz: the oversampler's half-band decimation
// filter has its transition band straddling Nyquist, so partials just above
// Nyquist fold back into the inaudible >20 kHz region at reduced level by
// design — that's a property of the filter topology, not an aliasing defect.
float worstAliasDb (const std::vector<float>& samples, float fundamental)
{
    REQUIRE (samples.size() == static_cast<size_t> (kFftSize));

    std::vector<float> fftData (2 * kFftSize, 0.0f);
    std::copy (samples.begin(), samples.end(), fftData.begin());

    // Blackman-Harris: sidelobes ~-92 dB, so window leakage can't masquerade
    // as aliasing at the levels we assert on.
    juce::dsp::WindowingFunction<float> window (
        kFftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);
    window.multiplyWithWindowingTable (fftData.data(), kFftSize);

    juce::dsp::FFT fft (kFftOrder);
    fft.performFrequencyOnlyForwardTransform (fftData.data());

    const float binHz = static_cast<float> (kSampleRate) / kFftSize;
    const int numBins = std::min (kFftSize / 2, static_cast<int> (20000.0f / binHz));

    // The Blackman-Harris main lobe spans ~8 bins; use a generous margin.
    const int exclusionBins = 16;

    float maxHarmonic = 0.0f;
    float maxAlias = 0.0f;
    float worstAliasFreq = 0.0f;
    for (int bin = exclusionBins; bin < numBins; ++bin)
    {
        const float freq = static_cast<float> (bin) * binHz;
        const float harmonicRatio = freq / fundamental;
        const float distanceToHarmonic = std::abs (harmonicRatio - std::round (harmonicRatio)) * fundamental;

        if (distanceToHarmonic <= exclusionBins * binHz)
        {
            maxHarmonic = std::max (maxHarmonic, fftData[static_cast<size_t> (bin)]);
        }
        else if (fftData[static_cast<size_t> (bin)] > maxAlias)
        {
            maxAlias = fftData[static_cast<size_t> (bin)];
            worstAliasFreq = freq;
        }
    }

    REQUIRE (maxHarmonic > 0.0f);
    UNSCOPED_INFO ("worst alias component at " << worstAliasFreq << " Hz");
    return juce::Decibels::gainToDecibels (maxAlias / maxHarmonic);
}

// A naive (non-band-limited) sawtooth for comparison: this is what the DCO's
// PolyBLEP + oversampling is up against.
std::vector<float> renderNaiveSaw (float frequency)
{
    std::vector<float> samples (kFftSize);
    double phase = 0.0;
    const double increment = frequency / kSampleRate;
    for (auto& s : samples)
    {
        s = static_cast<float> (2.0 * phase - 1.0);
        phase += increment;
        if (phase >= 1.0)
            phase -= 1.0;
    }
    return samples;
}
} // namespace

TEST_CASE ("DCO sawtooth is band-limited at high frequencies", "[dco]")
{
    PluginProcessor plugin;

    // A7 — high enough that a naive saw aliases audibly
    const float frequency = 3520.0f;

    const float dcoAlias = worstAliasDb (renderDCO (plugin, frequency, 0.0f), frequency);
    const float naiveAlias = worstAliasDb (renderNaiveSaw (frequency), frequency);

    INFO ("DCO worst alias: " << dcoAlias << " dB, naive saw: " << naiveAlias << " dB");

    // Absolute bar: aliased partials stay well below the harmonic content
    // (measured ~-67 dB with two-sided polyBLEP + 4x oversampling).
    CHECK (dcoAlias < -60.0f);

    // Relative bar: dramatically better than a non-band-limited saw.
    CHECK (dcoAlias < naiveAlias - 40.0f);
}

TEST_CASE ("DCO pulse wave is band-limited at high frequencies", "[dco]")
{
    PluginProcessor plugin;

    const float frequency = 3520.0f;
    const float aliasDb = worstAliasDb (renderDCO (plugin, frequency, 1.0f), frequency);

    INFO ("Pulse worst alias: " << aliasDb << " dB");
    // Measured ~-70 dB with two-sided polyBLEP + 4x oversampling
    CHECK (aliasDb < -60.0f);
}

TEST_CASE ("Sub-oscillator square and saw are band-limited", "[dco]")
{
    PluginProcessor plugin;

    // The sub runs an octave below the main oscillator frequency
    const float frequency = 3520.0f;
    const float subFundamental = frequency / 2.0f;

    SECTION ("square")
    {
        const float aliasDb = worstAliasDb (
            renderDCO (plugin, frequency, 0.0f, 2.0f / 3.0f), subFundamental);
        INFO ("Sub square worst alias: " << aliasDb << " dB");
        CHECK (aliasDb < -60.0f);
    }

    SECTION ("saw")
    {
        const float aliasDb = worstAliasDb (
            renderDCO (plugin, frequency, 0.0f, 1.0f), subFundamental);
        INFO ("Sub saw worst alias: " << aliasDb << " dB");
        CHECK (aliasDb < -60.0f);
    }
}
