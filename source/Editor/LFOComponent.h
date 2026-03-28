#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../Modulation/LFOData.h"
#include "../Modulation/MyLFO.h"
#include "../Theme.h"
#include "Utility/PaintHelpers.h"
#include "LFORateComponent.h"
#include "Utility/AnimationFrameSource.h"
#include "Utility/CustomToggleComponent.h"
#include "../Utility/Parameters.h"

class LFOComponent final : public juce::Component,
                           public LFOData::Listener,
                           public AnimationFrameSource::Listener
{
public:
    LFOComponent (std::shared_ptr<LFOData> lfoData, juce::AudioProcessorValueTreeState& parameters, bool showRateSlider = true, int lfoIndex = 1,
                  std::shared_ptr<MyLFO*> lfoPtr = nullptr, AnimationFrameSource* animSource = nullptr);
    ~LFOComponent() override;

    void rebind (std::shared_ptr<LFOData> newData, int newLfoIndex, std::shared_ptr<MyLFO*> newLfoPtr = nullptr);

    void lfoDataChanged() override;
    void onAnimationFrame() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void setBottomTabReserve (int pixels) { bottomTabReserve = pixels; resized(); repaint(); }

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;

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

    static juce::String paramId (const juce::String& base, int index)
    {
        return ParamIDs::lfoParamID (index, base.toRawUTF8());
    }

    std::shared_ptr<LFOData> lfoData;
    juce::AudioProcessorValueTreeState& apvts;
    const bool hasRateSlider;
    int lfoIndex;

    std::shared_ptr<MyLFO*> myLFO;
    AnimationFrameSource* animSource = nullptr;
    float currentPhase = 0.0f;
    bool showPhaseIndicator = false;

    // New rate display and toggle components
    std::unique_ptr<LFORateComponent> rateComponent;
    std::unique_ptr<CustomToggleComponent> bpmToggle;
    std::unique_ptr<CustomToggleComponent> hostToggle;
    std::unique_ptr<CustomToggleComponent> monoToggle;

    DragTarget dragTarget = DragTarget::None;
    int selectedPointIndex = -1;
    int selectedCurveIndex = -1;
    bool isStuck = false;
    float stickAccumulator = 0.0f;
    int hoveredPointIndex = -1;
    int hoveredCurveIndex = -1;

    int bottomTabReserve = 0;

    static constexpr float kPointRadius = 5.0f;
    static constexpr float kHitRadius = 12.0f;
    static constexpr float kCurveHandleRadius = 4.0f;
    static constexpr float kOuterPadding = 4.0f;
    static constexpr float kInnerPadding = 4.0f;
    static constexpr float kTotalPadding = kOuterPadding + kInnerPadding;
    static constexpr float kControlColumnWidth = 50.0f;
    static constexpr float kStickThreshold = 15.0f;
    static constexpr float kShapeButtonSize = 24.0f;
    static constexpr float kShapeStripWidth = 32.0f;

    // ---------------------------------------------------------------------------------
    // Shape preset button (iconId: 0=Sine, 1=Triangle, 2=Saw, 3=Square, 4=FlipH, 5=FlipV)
    // ---------------------------------------------------------------------------------
    class ShapePresetButton : public juce::Component
    {
    public:
        ShapePresetButton (int icon, std::function<void()> onClick)
            : iconId (icon), clickCallback (std::move (onClick))
        {
            setRepaintsOnMouseActivity (true);
        }

        void paint (juce::Graphics& g) override
        {
            const auto fullBounds = getLocalBounds().toFloat();

            PaintHelpers::drawHoverBox (g, fullBounds, isMouseOver());
            g.setColour (TEXT_COLOR.withAlpha (Style::alphaBorder));
            g.drawRoundedRectangle (fullBounds.reduced (0.5f), Style::radiusMedium, 1.0f);

            const auto bounds = fullBounds.reduced (5.0f);
            juce::Path icon;

            switch (iconId)
            {
                case 0: // Sine
                {
                    const float w = bounds.getWidth();
                    const float h = bounds.getHeight();
                    const float x0 = bounds.getX();
                    const float y0 = bounds.getY();
                    icon.startNewSubPath (x0, y0 + h * 0.5f);
                    icon.cubicTo (x0 + w * 0.25f, y0, x0 + w * 0.25f, y0, x0 + w * 0.5f, y0 + h * 0.5f);
                    icon.cubicTo (x0 + w * 0.75f, y0 + h, x0 + w * 0.75f, y0 + h, x0 + w, y0 + h * 0.5f);
                    break;
                }
                case 1: // Triangle
                {
                    icon.startNewSubPath (bounds.getX(), bounds.getBottom());
                    icon.lineTo (bounds.getCentreX(), bounds.getY());
                    icon.lineTo (bounds.getRight(), bounds.getBottom());
                    break;
                }
                case 2: // Saw (RampDown)
                {
                    icon.startNewSubPath (bounds.getX(), bounds.getY());
                    icon.lineTo (bounds.getRight(), bounds.getBottom());
                    icon.lineTo (bounds.getRight(), bounds.getY());
                    break;
                }
                case 3: // Square
                {
                    const float x0 = bounds.getX();
                    const float y0 = bounds.getY();
                    const float xMid = bounds.getCentreX();
                    const float xEnd = bounds.getRight();
                    const float yBot = bounds.getBottom();
                    icon.startNewSubPath (x0, yBot);
                    icon.lineTo (x0, y0);
                    icon.lineTo (xMid, y0);
                    icon.lineTo (xMid, yBot);
                    icon.lineTo (xEnd, yBot);
                    break;
                }
                case 4: // Horizontal flip (<-->)
                {
                    const float cx = bounds.getCentreX();
                    const float cy = bounds.getCentreY();
                    const float hw = bounds.getWidth() * 0.45f;
                    const float ah = bounds.getHeight() * 0.25f;
                    // Horizontal line
                    icon.startNewSubPath (cx - hw, cy);
                    icon.lineTo (cx + hw, cy);
                    // Left arrowhead
                    icon.startNewSubPath (cx - hw + ah, cy - ah);
                    icon.lineTo (cx - hw, cy);
                    icon.lineTo (cx - hw + ah, cy + ah);
                    // Right arrowhead
                    icon.startNewSubPath (cx + hw - ah, cy - ah);
                    icon.lineTo (cx + hw, cy);
                    icon.lineTo (cx + hw - ah, cy + ah);
                    break;
                }
                case 5: // Vertical flip (double vertical arrow)
                {
                    const float cx = bounds.getCentreX();
                    const float cy = bounds.getCentreY();
                    const float hh = bounds.getHeight() * 0.45f;
                    const float aw = bounds.getWidth() * 0.25f;
                    // Vertical line
                    icon.startNewSubPath (cx, cy - hh);
                    icon.lineTo (cx, cy + hh);
                    // Top arrowhead
                    icon.startNewSubPath (cx - aw, cy - hh + aw);
                    icon.lineTo (cx, cy - hh);
                    icon.lineTo (cx + aw, cy - hh + aw);
                    // Bottom arrowhead
                    icon.startNewSubPath (cx - aw, cy + hh - aw);
                    icon.lineTo (cx, cy + hh);
                    icon.lineTo (cx + aw, cy + hh - aw);
                    break;
                }
                default:
                    break;
            }

            g.setColour (TEXT_COLOR);
            g.strokePath (icon, juce::PathStrokeType (1.5f));
        }

        void mouseUp (const juce::MouseEvent& e) override
        {
            if (getLocalBounds().contains (e.x, e.y) && clickCallback)
                clickCallback();
        }

    private:
        int iconId;
        std::function<void()> clickCallback;
    };

    ShapePresetButton sineButton, triangleButton, sawButton, squareButton;
    ShapePresetButton flipHButton, flipVButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LFOComponent)
};
