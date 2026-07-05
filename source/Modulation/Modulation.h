//
// Created by Tyler Teuber on 3/24/25.
//
#pragma once

#include "../Utility/LockFreeQueue.h"

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

    void setOutputIndex (int idx) { outputIndex = idx; }
    [[nodiscard]] int getOutputIndex() const { return outputIndex; }

private:
    juce::String id;
    juce::String name;
    int outputIndex = -1;
};

struct Modulation
{
    ModSource* source;
    float depth;
    bool isBipolar;
    int slotIndex;
};

enum class ModCommandType
{
    Add,
    Remove,
    Update
};

struct ModCommand
{
    ModCommandType type;
    ModSource* source;
    ModDestination* destination;
    float depth;
    bool isBipolar;
    int slotIndex;
};

class ModMatrix
{
public:
    ModMatrix() = default;

    void setSourceOutputArray (std::atomic<float>* arr) { sourceOutputs = arr; }
    void setDestOutputArray (std::atomic<float>* arr) { destOutputs = arr; }

    void initDestination (ModDestination* destination)
    {
        if (!matrix.contains (destination))
            matrix.emplace (destination, std::vector<Modulation> {});
    }

    void addModulation (ModDestination* destination, ModSource* source, float depth, bool isBipolar, int slotIndex);

    void removeModulation (int slotIndex, ModDestination* destination);

    void updateModulation (int slotIndex, ModDestination* destination, float depth);

    // Thread-safe queuing methods (call from UI thread)
    void queueAddModulation (ModDestination* destination, ModSource* source, float depth, bool isBipolar, int slotIndex);
    void queueRemoveModulation (int slotIndex, ModDestination* destination);
    void queueUpdateModulation (int slotIndex, ModDestination* destination, float depth);

    // Process pending commands (call at start of audio block)
    void processPendingCommands() noexcept;

    void prepare (const juce::dsp::ProcessSpec& spec) const;

    void processSample() const noexcept;

    void resetOutputs() const noexcept;

private:
    std::map<ModDestination*, std::vector<Modulation>> matrix = {};
    LockFreeQueue<ModCommand, 64> commandQueue;
    std::atomic<float>* sourceOutputs = nullptr;
    std::atomic<float>* destOutputs = nullptr;
};
