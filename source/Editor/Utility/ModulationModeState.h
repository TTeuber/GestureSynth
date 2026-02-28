//
// Shared state for modulation mode
//

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class ModulationModeState
{
public:
    enum class Mode { Normal, Modulation };

    struct Listener
    {
        virtual ~Listener() = default;
        virtual void modulationModeChanged (Mode /*newMode*/) {}
        virtual void targetSourceChanged (const juce::String& /*newSourceID*/) {}
    };

    void addListener (Listener* l) { listeners.add (l); }
    void removeListener (Listener* l) { listeners.remove (l); }

    // --- Mode ---
    Mode getMode() const { return currentMode; }
    bool isModulationMode() const { return currentMode == Mode::Modulation; }

    void setMode (Mode m)
    {
        if (currentMode != m)
        {
            currentMode = m;
            listeners.call (&Listener::modulationModeChanged, currentMode);
        }
    }

    void toggleMode()
    {
        setMode (currentMode == Mode::Normal ? Mode::Modulation : Mode::Normal);
    }

    // --- Target source ---
    const juce::String& getTargetSourceID() const { return targetSourceID; }

    void setTargetSource (const juce::String& sourceID)
    {
        if (targetSourceID != sourceID)
        {
            targetSourceID = sourceID;
            listeners.call (&Listener::targetSourceChanged, targetSourceID);
        }
    }

    // --- modTree pointer (set by PluginEditor) ---
    void setModTree (juce::ValueTree* tree) { modTree = tree; }
    juce::ValueTree* getModTree() const { return modTree; }

    // --- modTree slot helpers ---

    // Find slot index with matching source and dest (-1 if not found)
    int findSlotIndex (const juce::String& sourceID, const juce::String& destID) const
    {
        if (modTree == nullptr)
            return -1;
        for (int i = 0; i < modTree->getNumChildren(); ++i)
        {
            auto child = modTree->getChild (i);
            if (child.getProperty ("source").toString() == sourceID
                && child.getProperty ("destination").toString() == destID)
                return i;
        }
        return -1;
    }

    // Find first free slot (source == "None" && destination == "None"), -1 if full
    int findFreeSlot() const
    {
        if (modTree == nullptr)
            return -1;
        for (int i = 0; i < modTree->getNumChildren(); ++i)
        {
            auto child = modTree->getChild (i);
            if (child.getProperty ("source").toString() == "None"
                && child.getProperty ("destination").toString() == "None")
                return i;
        }
        return -1;
    }

    // Return existing slot index or occupy a free slot; -1 if full
    int getOrCreateSlot (const juce::String& sourceID, const juce::String& destID)
    {
        int idx = findSlotIndex (sourceID, destID);
        if (idx >= 0)
            return idx;

        idx = findFreeSlot();
        if (idx >= 0 && modTree != nullptr)
        {
            auto child = modTree->getChild (idx);
            child.setProperty ("source", sourceID, nullptr);
            child.setProperty ("destination", destID, nullptr);

            // LFOs and keyboard are bipolar by default
            if (sourceID.startsWith ("lfo") || sourceID == "keyboard")
                child.setProperty ("isBipolar", true, nullptr);
        }
        return idx;
    }

    float getDepth (const juce::String& sourceID, const juce::String& destID) const
    {
        int idx = findSlotIndex (sourceID, destID);
        if (idx >= 0 && modTree != nullptr)
            return static_cast<float> (modTree->getChild (idx).getProperty ("depth"));
        return 0.0f;
    }

    void setDepth (const juce::String& sourceID, const juce::String& destID, float depth)
    {
        int idx = findSlotIndex (sourceID, destID);
        if (idx >= 0 && modTree != nullptr)
            modTree->getChild (idx).setProperty ("depth", depth, nullptr);
    }

    bool isBipolar (const juce::String& sourceID, const juce::String& destID) const
    {
        int idx = findSlotIndex (sourceID, destID);
        if (idx >= 0 && modTree != nullptr)
            return static_cast<bool> (modTree->getChild (idx).getProperty ("isBipolar"));
        return false;
    }

    bool isBypassed (const juce::String& sourceID, const juce::String& destID) const
    {
        int idx = findSlotIndex (sourceID, destID);
        if (idx >= 0 && modTree != nullptr)
            return static_cast<bool> (modTree->getChild (idx).getProperty ("bypassed"));
        return false;
    }

    void setBypassed (const juce::String& sourceID, const juce::String& destID, bool bypassed)
    {
        int idx = findSlotIndex (sourceID, destID);
        if (idx >= 0 && modTree != nullptr)
            modTree->getChild (idx).setProperty ("bypassed", bypassed, nullptr);
    }

    void toggleBipolar (const juce::String& sourceID, const juce::String& destID)
    {
        int idx = findSlotIndex (sourceID, destID);
        if (idx >= 0 && modTree != nullptr)
        {
            auto child = modTree->getChild (idx);
            bool current = static_cast<bool> (child.getProperty ("isBipolar"));
            child.setProperty ("isBipolar", !current, nullptr);
        }
    }

    void clearSlot (const juce::String& sourceID, const juce::String& destID)
    {
        int idx = findSlotIndex (sourceID, destID);
        if (idx >= 0 && modTree != nullptr)
        {
            auto child = modTree->getChild (idx);
            child.setProperty ("source", "None", nullptr);
            child.setProperty ("destination", "None", nullptr);
            child.setProperty ("depth", 0.0f, nullptr);
            child.setProperty ("isBipolar", false, nullptr);
            child.setProperty ("bypassed", false, nullptr);
        }
    }

private:
    Mode currentMode = Mode::Normal;
    juce::String targetSourceID { "lfo1" };
    juce::ValueTree* modTree = nullptr;
    juce::ListenerList<Listener> listeners;
};
