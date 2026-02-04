//
// Created by Tyler Teuber on 3/24/25.
//

#include "Modulation.h"

void ModMatrix::addModulation (ModDestination* destination, ModSource* source, float depth, bool isBipolar)
{
    if (matrix.contains (destination))
        matrix[destination].emplace_back (source, depth, isBipolar);
    else
        matrix.emplace (destination, std::vector<Modulation> { { source, depth, isBipolar } });
}
void ModMatrix::removeModulation (ModSource* source, ModDestination* destination)
{
    if (matrix.contains (destination))
    {
        auto& mods = matrix[destination];
        std::erase_if (mods, [&source] (const Modulation& mod) { return mod.source == source; });
        if (mods.empty())
            matrix.erase (destination);
    }
}
void ModMatrix::updateModulation (const ModSource* source, ModDestination* destination, float depth)
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

void ModMatrix::queueAddModulation (ModDestination* destination, ModSource* source, float depth, bool isBipolar)
{
    ModCommand cmd { ModCommandType::Add, source, destination, depth, isBipolar };
    commandQueue.push (cmd);
}

void ModMatrix::queueRemoveModulation (ModSource* source, ModDestination* destination)
{
    ModCommand cmd { ModCommandType::Remove, source, destination, 0.0f, false };
    commandQueue.push (cmd);
}

void ModMatrix::queueUpdateModulation (ModSource* source, ModDestination* destination, float depth)
{
    ModCommand cmd { ModCommandType::Update, source, destination, depth, false };
    commandQueue.push (cmd);
}

void ModMatrix::processPendingCommands() noexcept
{
    ModCommand cmd;
    while (commandQueue.pop (cmd))
    {
        switch (cmd.type)
        {
            case ModCommandType::Add:
                addModulation (cmd.destination, cmd.source, cmd.depth, cmd.isBipolar);
                break;
            case ModCommandType::Remove:
                removeModulation (cmd.source, cmd.destination);
                break;
            case ModCommandType::Update:
                updateModulation (cmd.source, cmd.destination, cmd.depth);
                break;
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
        float value = destination->getRawParameterValue();
        for (const auto& [source, depth, isBipolar] : mods)
        {
            if (isBipolar)
                value += (source->getNextValue() * 2.0f - 1.0f) * depth;
            else
                value += source->getNextValue() * depth;
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
