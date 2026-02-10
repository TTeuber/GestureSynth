#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../Modulation/LFOData.h"
#include "../Theme.h"

class LFOComponent final : public juce::Component,
                           public LFOData::Listener,
                           public juce::AudioProcessorValueTreeState::Listener
{
public:
    LFOComponent (std::shared_ptr<LFOData> lfoData, juce::AudioProcessorValueTreeState& parameters, bool showRateSlider = true, int lfoIndex = 1);
    ~LFOComponent() override;

    void rebind (std::shared_ptr<LFOData> newData, int newLfoIndex);

    void lfoDataChanged() override;
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;

private:
    enum class DragTarget
    {
        None,
        Point,
        CurveHandle
    };

    // Coordinate conversion helpers
    juce::Rectangle<float> getGraphBounds() const;
    float positionToX (float position) const;
    float valueToY (float value) const;
    float xToPosition (float x) const;
    float yToValue (float y) const;

    // Hit testing
    int hitTestPoint (float x, float y) const;
    int hitTestCurveHandle (float x, float y) const;
    juce::Point<float> getCurveHandlePosition (size_t segmentIndex) const;

    void updateSyncVisibility (bool tempoSyncOn);

    static juce::String paramId (const juce::String& base, int index)
    {
        return "lfo" + juce::String (index) + base;
    }

    std::shared_ptr<LFOData> lfoData;
    juce::AudioProcessorValueTreeState& apvts;
    const bool hasRateSlider;
    int lfoIndex;

    juce::Slider rateSlider;
    juce::Label rateLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rateAttachment;

    // Tempo sync controls (only used when hasRateSlider == true)
    juce::ToggleButton tempoSyncToggle { "Sync" };
    juce::ComboBox noteDivisionCombo;
    juce::ToggleButton beatSyncToggle { "Beat Lock" };
    juce::Slider bpmSlider;
    juce::Label bpmLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> tempoSyncAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> noteDivisionAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> beatSyncAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bpmAttachment;

    DragTarget dragTarget = DragTarget::None;
    int selectedPointIndex = -1;
    int selectedCurveIndex = -1;
    bool isStuck = false;
    float stickAccumulator = 0.0f;

    static constexpr float kPointRadius = 5.0f;
    static constexpr float kHitRadius = 12.0f;
    static constexpr float kCurveHandleRadius = 4.0f;
    static constexpr float kMargin = 10.0f;
    static constexpr float kSliderHeight = 30.0f;
    static constexpr float kStickThreshold = 15.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LFOComponent)
};
