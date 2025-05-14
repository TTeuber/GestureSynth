//
// Created by Tyler Teuber on 3/24/25.
//
#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>

class ModSource
{
public:
    ModSource (const juce::StringRef i, const juce::StringRef n) : id (i), name (n)
    {
    }
    virtual ~ModSource() = default;
    virtual void prepare (const juce::dsp::ProcessSpec& spec) { sampleRate = static_cast<float> (spec.sampleRate); }
    [[nodiscard]] virtual float getCurrentValue() const noexcept { return -1.0f; }
    virtual float getNextValue() noexcept = 0;
    [[nodiscard]] virtual juce::String getID() const noexcept { return id; }
    [[nodiscard]] virtual juce::String getName() const noexcept { return name; }

protected:
    float sampleRate = 44100.0f;
    juce::String id;
    juce::String name;
};

class ModDestination
{
public:
    ModDestination (const juce::StringRef i, const juce::StringRef n) : id (i), name (n) {}
    virtual ~ModDestination() = default;
    [[nodiscard]] virtual float getRawParameterValue() const noexcept = 0;
    virtual void setBaseValue (float value) noexcept = 0;
    virtual void setCurrentValue (float value) noexcept = 0;
    [[nodiscard]] virtual float getBaseValue() const noexcept = 0;
    [[nodiscard]] virtual float getCurrentValue() const noexcept = 0;
    [[nodiscard]] virtual float getMinValue() const noexcept = 0;
    [[nodiscard]] virtual float getMaxValue() const noexcept = 0;
    [[nodiscard]] virtual juce::NormalisableRange<float> getRange() const noexcept = 0;
    [[nodiscard]] virtual juce::String getID() const noexcept { return id; }
    [[nodiscard]] virtual juce::StringRef getName() const noexcept { return name; }

private:
    juce::String id;
    juce::String name;
};

struct Modulation
{
    ModSource* source;
    float depth;
    bool isBipolar;
};

class ModMatrix
{
public:
    ModMatrix() = default;

    void initDestination (ModDestination* destination)
    {
        if (!matrix.contains (destination))
            matrix.emplace (destination, std::vector<Modulation> {});
    }

    void addModulation (ModDestination* destination, ModSource* source, float depth, bool isBipolar = false);

    void removeModulation (ModSource* source, ModDestination* destination);

    void updateModulation (const ModSource* source, ModDestination* destination, float depth);

    void prepare (const juce::dsp::ProcessSpec& spec) const;

    void processSample() const noexcept;

    void debug();

private:
    std::map<ModDestination*, std::vector<Modulation>> matrix = {};
};
