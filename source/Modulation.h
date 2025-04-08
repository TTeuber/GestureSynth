//
// Created by Tyler Teuber on 3/24/25.
//
#pragma once
#include "MatrixComponent.h"
#include <juce_dsp/juce_dsp.h>
#include <utility>
#include <vector>

class ModSource
{
public:
    ModSource (juce::StringRef i, juce::StringRef n) : id (i), name (n)
    {
    }
    virtual ~ModSource() = default;
    void prepare (const juce::dsp::ProcessSpec& spec) { sampleRate = static_cast<float> (spec.sampleRate); }
    // virtual void reset() noexcept;
    // virtual float getNextValue() noexcept = 0;
    virtual float getCurrentValue() const noexcept { return -1.0f; }
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
    ModDestination (juce::StringRef i, const juce::StringRef n) : id (i), name (n) {}
    virtual ~ModDestination() = default;
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
    std::shared_ptr<ModSource> source;
    float depth;
    bool isBipolar;
};

class ModMatrix
{
public:
    ModMatrix() = default;

    void addModulation (ModDestination* destination, std::shared_ptr<ModSource> source, float depth, bool isBipolar = false)
    {
        if (matrix.contains (destination))
            matrix[destination].emplace_back (source, depth, isBipolar);
        else
            matrix.emplace (destination, std::vector<Modulation> { { source, depth, isBipolar } });
    }

    void removeModulation (std::shared_ptr<ModSource> source, ModDestination* destination)
    {
        if (matrix.contains (destination))
        {
            auto& mods = matrix[destination];
            std::erase_if (mods, [&source] (const Modulation& mod) { return &mod.source == &source; });
            if (mods.empty())
                matrix.erase (destination);
        }
    }

    void updateModulation (std::shared_ptr<ModSource> source, ModDestination* destination, float depth)
    {
        if (matrix.contains (destination))
        {
            auto& mods = matrix[destination];
            for (auto& mod : mods)
            {
                if (mod.source == source)
                {
                    mod.depth = depth;
                    break;
                }
            }
        }
    }

    void prepare (const juce::dsp::ProcessSpec& spec) const
    {
        for (const auto& [_, mods] : matrix)
        {
            for (auto& mod : mods)
                mod.source->prepare (spec);
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
                    value += (source->getNextValue() * 2.0f - 1.0f) * depth + destination->getRange().convertTo0to1 (destination->getBaseValue());
                else
                    value += source->getNextValue() * depth + destination->getRange().convertTo0to1 (destination->getBaseValue());
            }

            value = juce::jlimit (0.0f, 1.0f, value);
            destination->setCurrentValue (destination->getRange().convertFrom0to1 (value));
        }
    }

    void debug()
    {
        for (const auto& [destination, mods] : matrix)
        {
            DBG ("Destination: " << destination->getName() << "\n");
            for (const auto& mod : mods)
            {
                DBG ("  Source: " << mod.source->getName() << ", Depth: " << mod.depth << ", Bipolar: " << (mod.isBipolar ? "true" : "false") << "\n");
            }
        }
    }

private:
    std::map<ModDestination*, std::vector<Modulation>> matrix = {};
};
