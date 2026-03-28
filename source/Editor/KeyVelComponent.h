//
// KeyVelComponent.h
//
#pragma once

#include "../Modulation/MyADSR.h"
#include "../Modulation/VelocitySource.h"
#include "../Theme.h"
#include "Utility/AnimationFrameSource.h"
#include "Utility/PaintHelpers.h"
#include "Utility/ModulationModeState.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

class KeyVelComponent final : public juce::Component,
                              public ModulationModeState::Listener,
                              public AnimationFrameSource::Listener
{
public:
    KeyVelComponent (juce::AudioProcessorValueTreeState& apvts,
                     juce::UndoManager* um,
                     std::atomic<int>* gestureCount,
                     ModulationModeState* modState = nullptr,
                     std::atomic<float>* velocityRaw = nullptr,
                     std::atomic<float>* keyboardRaw = nullptr,
                     AnimationFrameSource* animSource = nullptr,
                     std::shared_ptr<MyADSR*> ampEnv = nullptr);
    ~KeyVelComponent() override;

    // ModulationModeState::Listener
    void modulationModeChanged (ModulationModeState::Mode newMode) override;
    void targetSourceChanged (const juce::String& newSourceID) override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;

private:
    void updateCurveFromDrag (const juce::MouseEvent& e);
    float getCurveParam() const;
    juce::RangedAudioParameter* getCurrentParam() const;
    juce::Rectangle<float> getGraphArea() const;
    bool isOverHandle (const juce::MouseEvent& e) const;

    juce::AudioProcessorValueTreeState& parameters;
    juce::UndoManager* undoManager;
    std::atomic<int>* activeGestureCount;
    ModulationModeState* modModeState = nullptr;

    juce::RangedAudioParameter* velCurveParam = nullptr;
    juce::RangedAudioParameter* keyCurveParam = nullptr;

    int activeTab = 0; // 0 = Vel, 1 = Key
    int bottomTabReserve = 0;
    bool dragging = false;
    bool handleHovered = false;

    std::atomic<float>* velocityRawPtr = nullptr;
    std::atomic<float>* keyboardRawPtr = nullptr;
    AnimationFrameSource* animSource = nullptr;
    std::shared_ptr<MyADSR*> ampEnvPtr;
    float currentInputValue = 0.0f;
    bool wasNoteActive = false;

    void onAnimationFrame() override;

public:
    void setActiveTab (int index) { activeTab = index; repaint(); }
    void setBottomTabReserve (int pixels) { bottomTabReserve = pixels; resized(); repaint(); }
};
