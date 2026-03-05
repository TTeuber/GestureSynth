//
// Lightweight context struct passed to UI components to reduce constructor bloat.
//

#pragma once

#include <atomic>

namespace juce { class UndoManager; }
class ModulationModeState;

struct UIContext
{
    juce::UndoManager* undoManager = nullptr;
    std::atomic<int>* gestureCount = nullptr;
    ModulationModeState* modModeState = nullptr;
};
