//
// Created by Tyler Teuber on 3/24/25.
//

#include "Modulation.h"

void ModMatrix::addModulation (ModDestination* destination, std::shared_ptr<ModSource> source, float depth, bool isBipolar)
{
    if (matrix.contains (destination))
        matrix[destination].emplace_back (source, depth, isBipolar);
    else
        matrix.emplace (destination, std::vector<Modulation> { { source, depth, isBipolar } });
}
void ModMatrix::removeModulation (std::shared_ptr<ModSource> source, ModDestination* destination)
{
    if (matrix.contains (destination))
    {
        auto& mods = matrix[destination];
        std::erase_if (mods, [&source] (const Modulation& mod) { return &mod.source == &source; });
        if (mods.empty())
            matrix.erase (destination);
    }
}
void ModMatrix::updateModulation (const std::shared_ptr<ModSource>& source, ModDestination* destination, float depth)
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

void ModMatrix::prepare (const juce::dsp::ProcessSpec& spec) const
{
    for (const auto& [_, mods] : matrix)
    {
        for (auto& mod : mods)
            mod.source->prepare (spec);
    }
}
void ModMatrix::processSample() const noexcept
{
    for (const auto& [destination, mods] : matrix)
    {
        // destination->setCurrentValue (destination->getBaseValue());
        // float value = destination->getRange().convertTo0to1 (destination->getBaseValue());
        float value = destination->getRawParameterValue();
        for (const auto& [source, depth, isBipolar] : mods)
        {
            if (isBipolar)
                // value += (source->getNextValue() * 2.0f - 1.0f) * depth + destination->getRange().convertTo0to1 (destination->getBaseValue());
                value += (source->getNextValue() * 2.0f - 1.0f) * depth;
            else
                value += source->getNextValue() * depth;
            // value += source->getNextValue() * depth + destination->getRange().convertTo0to1 (destination->getBaseValue());
        }

        value = juce::jlimit (0.0f, 1.0f, value);
        destination->setCurrentValue (destination->getRange().convertFrom0to1 (value));
    }
}

void ModMatrix::debug()
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
