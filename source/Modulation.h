//
// Created by Tyler Teuber on 3/24/25.
//
#pragma once
#include <juce_dsp/juce_dsp.h>
#include <vector>

class ModSource
{
public:
    virtual ~ModSource() = default;
    void prepare (const juce::dsp::ProcessSpec& spec) { sampleRate = static_cast<float> (spec.sampleRate); }
    // virtual void reset() noexcept;
    virtual float getNextValue() noexcept = 0;

protected:
    float sampleRate = 44100.0f;
};

class ModDestination
{
public:
    virtual ~ModDestination() = default;
    virtual void setBaseValue (float value) noexcept = 0;
    virtual void setCurrentValue (float value) noexcept = 0;
    [[nodiscard]] virtual float getBaseValue() const noexcept = 0;
    [[nodiscard]] virtual float getCurrentValue() const noexcept = 0;
    [[nodiscard]] virtual float getMinValue() const noexcept = 0;
    [[nodiscard]] virtual float getMaxValue() const noexcept = 0;
    [[nodiscard]] virtual juce::NormalisableRange<float> getRange() const noexcept = 0;
};

struct Modulation
{
    ModSource& source;
    float depth;
    bool isBipolar = false;
};

class ModMatrix
{
public:
    ModMatrix() = default;

    void addModulation (ModDestination* destination, ModSource& source, float depth, bool isBipolar = false)
    {
        if (matrix.contains (destination))
            matrix[destination].emplace_back (source, depth, isBipolar);
        else
            matrix.emplace (destination, std::vector<Modulation> { { source, depth, isBipolar } });
    }

    // void removeModulation (ModDestination* destination, ModSource& source)
    // {
    //     if (matrix.contains (destination))
    //     {
    //         auto& mods = matrix[destination];
    //         std::erase_if (mods, [&source] (const Modulation& mod) { return &mod.source == &source; });
    //     }
    // }

    void prepare (const juce::dsp::ProcessSpec& spec) const
    {
        for (const auto& [_, mods] : matrix)
        {
            for (auto& mod : mods)
                mod.source.prepare (spec);
        }
    }

    // void reset() const noexcept
    // {
    //     for (const auto& [_, mods] : matrix)
    //     {
    //         for (auto& mod : mods)
    //             mod.source.reset();
    //     }
    // }

    void processSample() const noexcept
    {
        for (const auto& [destination, mods] : matrix)
        {
            float value = 0.0f;
            for (const auto& [source, depth, isBipolar] : mods)
            {
                if (isBipolar)
                    value += (source.getNextValue() * 2.0f - 1.0f) * depth + destination->getRange().convertTo0to1 (destination->getBaseValue());
                else
                    value += source.getNextValue() * depth + destination->getRange().convertTo0to1 (destination->getBaseValue());
            }

            value = juce::jlimit (0.0f, 1.0f, value);
            destination->setCurrentValue (destination->getRange().convertFrom0to1 (value));
        }
    }

private:
    std::map<ModDestination*, std::vector<Modulation>> matrix = {};
};
