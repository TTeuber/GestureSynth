//
// KeyVelComponent.h
//
#pragma once

#include "../Modulation/VelocitySource.h"
#include "../Theme.h"
#include "Utility/ModulationModeState.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

class KeyVelComponent final : public juce::Component,
                              public ModulationModeState::Listener,
                              private juce::Timer
{
public:
    KeyVelComponent (juce::AudioProcessorValueTreeState& apvts,
                     juce::UndoManager* um,
                     std::atomic<int>* gestureCount,
                     ModulationModeState* modState = nullptr,
                     std::atomic<float>* velocityRaw = nullptr,
                     std::atomic<float>* keyboardRaw = nullptr);
    ~KeyVelComponent() override;

    // ModulationModeState::Listener
    void modulationModeChanged (ModulationModeState::Mode newMode) override;
    void targetSourceChanged (const juce::String& newSourceID) override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;

private:
    void updateCurveFromDrag (const juce::MouseEvent& e);
    float getCurveParam() const;
    juce::RangedAudioParameter* getCurrentParam() const;

    juce::AudioProcessorValueTreeState& parameters;
    juce::UndoManager* undoManager;
    std::atomic<int>* activeGestureCount;
    ModulationModeState* modModeState = nullptr;

    juce::RangedAudioParameter* velCurveParam = nullptr;
    juce::RangedAudioParameter* keyCurveParam = nullptr;

    int activeTab = 0; // 0 = Vel, 1 = Key
    bool dragging = false;

    std::atomic<float>* velocityRawPtr = nullptr;
    std::atomic<float>* keyboardRawPtr = nullptr;
    float currentInputValue = 0.0f;

    void timerCallback() override;

public:
    void setActiveTab (int index) { activeTab = index; repaint(); }
};
