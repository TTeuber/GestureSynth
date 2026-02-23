//
// Created by Tyler Teuber on 3/24/25.
//

#include "Modulation.h"

void ModMatrix::addModulation (ModDestination* destination, ModSource* source, float depth, bool isBipolar, int slotIndex)
{
    if (destination == nullptr || source == nullptr)
        return;

    if (matrix.contains (destination))
        matrix[destination].emplace_back (source, depth, isBipolar, slotIndex);
    else
        matrix.emplace (destination, std::vector<Modulation> { { source, depth, isBipolar, slotIndex } });
}
void ModMatrix::removeModulation (int slotIndex, ModDestination* destination)
{
    if (matrix.contains (destination))
    {
        auto& mods = matrix[destination];
        std::erase_if (mods, [slotIndex] (const Modulation& mod) { return mod.slotIndex == slotIndex; });
    }
}
void ModMatrix::updateModulation (int slotIndex, ModDestination* destination, float depth)
{
    if (matrix.contains (destination))
    {
        auto& mods = matrix[destination];
        for (auto& mod : mods)
        {
            if (mod.slotIndex == slotIndex)
            {
                mod.depth = depth;
                break;
            }
        }
    }
}

void ModMatrix::queueAddModulation (ModDestination* destination, ModSource* source, float depth, bool isBipolar, int slotIndex)
{
    ModCommand cmd { ModCommandType::Add, source, destination, depth, isBipolar, slotIndex };
    commandQueue.push (cmd);
}

void ModMatrix::queueRemoveModulation (int slotIndex, ModDestination* destination)
{
    ModCommand cmd { ModCommandType::Remove, nullptr, destination, 0.0f, false, slotIndex };
    commandQueue.push (cmd);
}

void ModMatrix::queueUpdateModulation (int slotIndex, ModDestination* destination, float depth)
{
    ModCommand cmd { ModCommandType::Update, nullptr, destination, depth, false, slotIndex };
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
                addModulation (cmd.destination, cmd.source, cmd.depth, cmd.isBipolar, cmd.slotIndex);
                break;
            case ModCommandType::Remove:
                removeModulation (cmd.slotIndex, cmd.destination);
                break;
            case ModCommandType::Update:
                updateModulation (cmd.slotIndex, cmd.destination, cmd.depth);
                break;
        }
    }
}

void ModMatrix::prepare (const juce::dsp::ProcessSpec& spec) const
{
    for (const auto& [_, mods] : matrix)
    {
        for (auto& mod : mods)
        {
            if (mod.source != nullptr)
                mod.source->prepare (spec);
        }
    }
}
void ModMatrix::processSample() const noexcept
{
    for (const auto& [destination, mods] : matrix)
    {
        if (destination == nullptr)
            continue;

        float value = destination->getRawParameterValue();
        for (const auto& [source, depth, isBipolar, slotIdx] : mods)
        {
            if (source == nullptr)
                continue;

            float rawSourceVal = source->getNextValue();
            if (sourceOutputs != nullptr)
                sourceOutputs[slotIdx].store (rawSourceVal, std::memory_order_relaxed);

            if (isBipolar)
                value += (rawSourceVal * 2.0f - 1.0f) * depth;
            else
                value += rawSourceVal * depth;
        }

        value = juce::jlimit (0.0f, 1.0f, value);

        if (destOutputs != nullptr && destination->getOutputIndex() >= 0)
            destOutputs[destination->getOutputIndex()].store (value, std::memory_order_relaxed);

        destination->setCurrentValue (destination->getRange().convertFrom0to1 (value));
    }
}

void ModMatrix::resetOutputs() const noexcept
{
    // Reset dest outputs to un-modulated base values
    if (destOutputs != nullptr)
    {
        for (const auto& [destination, mods] : matrix)
        {
            if (destination != nullptr && destination->getOutputIndex() >= 0)
                destOutputs[destination->getOutputIndex()].store (
                    destination->getRawParameterValue(), std::memory_order_relaxed);
        }
    }

    // Reset source outputs to 0 (hides DepthSlider indicator lines)
    if (sourceOutputs != nullptr)
    {
        for (const auto& [destination, mods] : matrix)
        {
            for (const auto& [source, depth, isBipolar, slotIdx] : mods)
                sourceOutputs[slotIdx].store (0.0f, std::memory_order_relaxed);
        }
    }
}

void ModMatrix::debug()
{
    for (const auto& [destination, mods] : matrix)
    {
        if (destination == nullptr)
            continue;

        DBG ("Destination: " << destination->getName() << "\n");
        for (const auto& mod : mods)
        {
            if (mod.source == nullptr)
                continue;

            DBG ("  Source: " << mod.source->getName() << ", Depth: " << mod.depth << ", Bipolar: " << (mod.isBipolar ? "true" : "false") << "\n");
        }
    }
}
