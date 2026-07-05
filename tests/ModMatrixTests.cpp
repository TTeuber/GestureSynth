#include <Modulation/Modulation.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

namespace
{
// Minimal stand-ins so ModMatrix math can be tested without an APVTS.
class FakeSource final : public ModSource
{
public:
    explicit FakeSource (float v = 0.0f) : ModSource ("fakeSrc", "Fake Source"), value (v) {}
    float getNextValue() noexcept override { return value; }
    [[nodiscard]] float getCurrentValue() const noexcept override { return value; }
    float value;
};

class FakeDestination final : public ModDestination
{
public:
    explicit FakeDestination (float base = 0.5f)
        : ModDestination ("fakeDest", "Fake Destination"), baseValue (base) {}

    [[nodiscard]] float getRawParameterValue() const noexcept override { return baseValue; }
    void setBaseValue (float value) noexcept override { baseValue = value; }
    void setCurrentValue (float value) noexcept override { currentValue = value; }
    [[nodiscard]] float getBaseValue() const noexcept override { return baseValue; }
    [[nodiscard]] float getCurrentValue() const noexcept override { return currentValue; }
    [[nodiscard]] float getMinValue() const noexcept override { return 0.0f; }
    [[nodiscard]] float getMaxValue() const noexcept override { return 1.0f; }
    [[nodiscard]] juce::NormalisableRange<float> getRange() const noexcept override
    {
        return { 0.0f, 1.0f };
    }

    float baseValue;
    float currentValue = -1.0f;
};
} // namespace

TEST_CASE ("Unipolar modulation adds source * depth to the base value", "[modmatrix]")
{
    ModMatrix matrix;
    FakeSource source (0.5f);
    FakeDestination dest (0.25f);

    matrix.addModulation (&dest, &source, 0.4f, false, 0);
    matrix.processSample();

    // 0.25 + 0.5 * 0.4 = 0.45 (identity range, so normalized == real value)
    CHECK_THAT (dest.getCurrentValue(), WithinAbs (0.45f, 1e-6f));
}

TEST_CASE ("Bipolar modulation maps source [0,1] to [-1,1] before applying depth", "[modmatrix]")
{
    ModMatrix matrix;
    FakeSource source;
    FakeDestination dest (0.5f);
    matrix.addModulation (&dest, &source, 0.4f, true, 0);

    SECTION ("source at 0 pushes fully negative")
    {
        source.value = 0.0f;
        matrix.processSample();
        CHECK_THAT (dest.getCurrentValue(), WithinAbs (0.1f, 1e-6f));
    }

    SECTION ("source at midpoint leaves the base value untouched")
    {
        source.value = 0.5f;
        matrix.processSample();
        CHECK_THAT (dest.getCurrentValue(), WithinAbs (0.5f, 1e-6f));
    }

    SECTION ("source at 1 pushes fully positive")
    {
        source.value = 1.0f;
        matrix.processSample();
        CHECK_THAT (dest.getCurrentValue(), WithinAbs (0.9f, 1e-6f));
    }
}

TEST_CASE ("Modulated values are clamped to the normalized range", "[modmatrix]")
{
    ModMatrix matrix;
    FakeSource source (1.0f);
    FakeDestination dest (0.9f);

    matrix.addModulation (&dest, &source, 1.0f, false, 0);
    matrix.processSample();
    CHECK_THAT (dest.getCurrentValue(), WithinAbs (1.0f, 1e-6f));

    dest.baseValue = 0.1f;
    source.value = 0.0f;

    ModMatrix bipolarMatrix;
    bipolarMatrix.addModulation (&dest, &source, 1.0f, true, 0);
    bipolarMatrix.processSample();
    CHECK_THAT (dest.getCurrentValue(), WithinAbs (0.0f, 1e-6f));
}

TEST_CASE ("Multiple modulations on one destination sum their offsets", "[modmatrix]")
{
    ModMatrix matrix;
    FakeSource sourceA (0.5f);
    FakeSource sourceB (1.0f);
    FakeDestination dest (0.2f);

    matrix.addModulation (&dest, &sourceA, 0.2f, false, 0);
    matrix.addModulation (&dest, &sourceB, 0.3f, false, 1);
    matrix.processSample();

    // 0.2 + 0.5*0.2 + 1.0*0.3 = 0.6
    CHECK_THAT (dest.getCurrentValue(), WithinAbs (0.6f, 1e-6f));
}

TEST_CASE ("Removing a modulation restores the unmodulated base value", "[modmatrix]")
{
    ModMatrix matrix;
    FakeSource source (1.0f);
    FakeDestination dest (0.3f);

    matrix.addModulation (&dest, &source, 0.5f, false, 3);
    matrix.processSample();
    CHECK_THAT (dest.getCurrentValue(), WithinAbs (0.8f, 1e-6f));

    matrix.removeModulation (3, &dest);
    matrix.processSample();
    CHECK_THAT (dest.getCurrentValue(), WithinAbs (0.3f, 1e-6f));
}

TEST_CASE ("Updating a modulation changes its depth in place", "[modmatrix]")
{
    ModMatrix matrix;
    FakeSource source (1.0f);
    FakeDestination dest (0.1f);

    matrix.addModulation (&dest, &source, 0.2f, false, 5);
    matrix.updateModulation (5, &dest, 0.7f);
    matrix.processSample();

    CHECK_THAT (dest.getCurrentValue(), WithinAbs (0.8f, 1e-6f));
}

TEST_CASE ("Null sources and destinations are rejected safely", "[modmatrix]")
{
    ModMatrix matrix;
    FakeSource source (1.0f);
    FakeDestination dest (0.5f);

    matrix.addModulation (nullptr, &source, 1.0f, false, 0);
    matrix.addModulation (&dest, nullptr, 1.0f, false, 0);
    matrix.removeModulation (0, &dest); // destination with no entry
    matrix.processSample();

    // Nothing was routed, so the destination was never written to.
    CHECK_THAT (dest.getCurrentValue(), WithinAbs (-1.0f, 1e-6f));
}

TEST_CASE ("Queued commands only take effect once processed on the audio thread", "[modmatrix]")
{
    ModMatrix matrix;
    FakeSource source (1.0f);
    FakeDestination dest (0.2f);

    matrix.queueAddModulation (&dest, &source, 0.5f, false, 0);
    matrix.processSample();
    CHECK_THAT (dest.getCurrentValue(), WithinAbs (-1.0f, 1e-6f)); // not yet applied

    matrix.processPendingCommands();
    matrix.processSample();
    CHECK_THAT (dest.getCurrentValue(), WithinAbs (0.7f, 1e-6f));

    matrix.queueUpdateModulation (0, &dest, 0.1f);
    matrix.processPendingCommands();
    matrix.processSample();
    CHECK_THAT (dest.getCurrentValue(), WithinAbs (0.3f, 1e-6f));

    matrix.queueRemoveModulation (0, &dest);
    matrix.processPendingCommands();
    matrix.processSample();
    CHECK_THAT (dest.getCurrentValue(), WithinAbs (0.2f, 1e-6f));
}
