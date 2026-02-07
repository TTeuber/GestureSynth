#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../Modulation/LFOData.h"
#include "../Theme.h"

class LFOComponent final : public juce::Component, public LFOData::Listener
{
public:
    LFOComponent (std::shared_ptr<LFOData> lfoData, juce::AudioProcessorValueTreeState& parameters);
    ~LFOComponent() override;

    void lfoDataChanged() override;

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

    std::shared_ptr<LFOData> lfoData;

    juce::Slider rateSlider;
    juce::Label rateLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rateAttachment;

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
