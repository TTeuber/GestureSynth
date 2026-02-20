#include "LFOComponent.h"
#include <algorithm>

LFOComponent::LFOComponent (std::shared_ptr<LFOData> data, juce::AudioProcessorValueTreeState& parameters, bool showRateSlider, int lfoIdx)
    : lfoData (std::move (data)),
      apvts (parameters),
      hasRateSlider (showRateSlider),
      lfoIndex (lfoIdx),
      sineButton (0, [this] { this->lfoData->setShape (LFOShape::Sine); this->lfoData->updateListeners(); }),
      triangleButton (1, [this] { this->lfoData->setShape (LFOShape::Triangle); this->lfoData->updateListeners(); }),
      sawButton (2, [this] { this->lfoData->setShape (LFOShape::RampDown); this->lfoData->updateListeners(); }),
      squareButton (3, [this] { this->lfoData->setShape (LFOShape::Square); this->lfoData->updateListeners(); }),
      flipHButton (4, [this] {
          auto pts = this->lfoData->getPoints();
          for (auto& p : pts)
          {
              p.position = 1.0f - p.position;
              p.curve = -p.curve;
          }
          std::reverse (pts.begin(), pts.end());
          this->lfoData->setPoints (pts);
      }),
      flipVButton (5, [this] {
          auto pts = this->lfoData->getPoints();
          for (auto& p : pts)
              p.value = 1.0f - p.value;
          this->lfoData->setPoints (pts);
      })
{
    lfoData->addListener (this);

    if (hasRateSlider)
    {
        // Rate slider (free Hz mode)
        rateSlider.setSliderStyle (juce::Slider::LinearHorizontal);
        rateSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
        rateSlider.setColour (juce::Slider::textBoxTextColourId, TEXT_COLOR);
        rateSlider.setColour (juce::Slider::textBoxBackgroundColourId, SECONDARY_COLOR);
        rateSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        rateSlider.setTextValueSuffix (" Hz");
        addAndMakeVisible (rateSlider);

        rateLabel.setText ("Rate", juce::dontSendNotification);
        rateLabel.setColour (juce::Label::textColourId, TEXT_COLOR);
        rateLabel.attachToComponent (&rateSlider, true);

        rateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (parameters, paramId ("Rate", lfoIndex), rateSlider);

        // Tempo sync toggle
        tempoSyncToggle.setColour (juce::ToggleButton::textColourId, TEXT_COLOR);
        tempoSyncToggle.setColour (juce::ToggleButton::tickColourId, TEXT_COLOR);
        addAndMakeVisible (tempoSyncToggle);
        tempoSyncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (parameters, paramId ("TempoSync", lfoIndex), tempoSyncToggle);

        // Note division combo
        const juce::StringArray divisions { "4/1", "2/1", "1/1", "1/2", "1/2T", "1/2D", "1/4", "1/4T", "1/4D", "1/8", "1/8T", "1/8D", "1/16", "1/16T", "1/16D", "1/32" };
        for (int i = 0; i < divisions.size(); ++i)
            noteDivisionCombo.addItem (divisions[i], i + 1);
        noteDivisionCombo.setColour (juce::ComboBox::backgroundColourId, SECONDARY_COLOR);
        noteDivisionCombo.setColour (juce::ComboBox::textColourId, TEXT_COLOR);
        noteDivisionCombo.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
        addChildComponent (noteDivisionCombo);
        noteDivisionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (parameters, paramId ("NoteDivision", lfoIndex), noteDivisionCombo);

        // Beat sync toggle
        beatSyncToggle.setColour (juce::ToggleButton::textColourId, TEXT_COLOR);
        beatSyncToggle.setColour (juce::ToggleButton::tickColourId, TEXT_COLOR);
        addChildComponent (beatSyncToggle);
        beatSyncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (parameters, paramId ("BeatSync", lfoIndex), beatSyncToggle);

        // Manual BPM slider
        bpmSlider.setSliderStyle (juce::Slider::LinearHorizontal);
        bpmSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
        bpmSlider.setColour (juce::Slider::textBoxTextColourId, TEXT_COLOR);
        bpmSlider.setColour (juce::Slider::textBoxBackgroundColourId, SECONDARY_COLOR);
        bpmSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        bpmSlider.setTextValueSuffix (" BPM");
        addChildComponent (bpmSlider);

        bpmLabel.setText ("BPM", juce::dontSendNotification);
        bpmLabel.setColour (juce::Label::textColourId, TEXT_COLOR);
        bpmLabel.attachToComponent (&bpmSlider, true);

        bpmAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (parameters, "manualBpm", bpmSlider);

        // Listen for tempo sync parameter changes
        parameters.addParameterListener (paramId ("TempoSync", lfoIndex), this);

        // Set initial visibility
        const bool syncOn = *parameters.getRawParameterValue (paramId ("TempoSync", lfoIndex)) > 0.5f;
        updateSyncVisibility (syncOn);
    }

    addAndMakeVisible (sineButton);
    addAndMakeVisible (triangleButton);
    addAndMakeVisible (sawButton);
    addAndMakeVisible (squareButton);
    addAndMakeVisible (flipHButton);
    addAndMakeVisible (flipVButton);

    setSize (300, 200);
}

LFOComponent::~LFOComponent()
{
    lfoData->removeListener (this);
    if (hasRateSlider)
        apvts.removeParameterListener (paramId ("TempoSync", lfoIndex), this);
}

void LFOComponent::rebind (std::shared_ptr<LFOData> newData, int newLfoIndex)
{
    // Remove old listeners
    lfoData->removeListener (this);
    if (hasRateSlider)
        apvts.removeParameterListener (paramId ("TempoSync", lfoIndex), this);

    // Reset all attachments (must be done before changing lfoIndex)
    rateAttachment.reset();
    tempoSyncAttachment.reset();
    noteDivisionAttachment.reset();
    beatSyncAttachment.reset();
    // bpmAttachment stays — manualBpm is shared

    // Swap data and index
    lfoData = std::move (newData);
    lfoIndex = newLfoIndex;

    // Re-add listeners
    lfoData->addListener (this);

    if (hasRateSlider)
    {
        // Recreate attachments with new parameter IDs
        rateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, paramId ("Rate", lfoIndex), rateSlider);
        tempoSyncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, paramId ("TempoSync", lfoIndex), tempoSyncToggle);
        noteDivisionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, paramId ("NoteDivision", lfoIndex), noteDivisionCombo);
        beatSyncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, paramId ("BeatSync", lfoIndex), beatSyncToggle);

        apvts.addParameterListener (paramId ("TempoSync", lfoIndex), this);
        const bool syncOn = *apvts.getRawParameterValue (paramId ("TempoSync", lfoIndex)) > 0.5f;
        updateSyncVisibility (syncOn);
    }

    repaint();
}

void LFOComponent::lfoDataChanged()
{
    repaint();
}

void LFOComponent::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == paramId ("TempoSync", lfoIndex))
    {
        const bool syncOn = newValue > 0.5f;
        juce::MessageManager::callAsync ([this, syncOn]() { updateSyncVisibility (syncOn); });
    }
}

void LFOComponent::updateSyncVisibility (bool tempoSyncOn)
{
    rateSlider.setVisible (!tempoSyncOn);
    rateLabel.setVisible (!tempoSyncOn);
    noteDivisionCombo.setVisible (tempoSyncOn);
    beatSyncToggle.setVisible (tempoSyncOn);
    bpmSlider.setVisible (tempoSyncOn);
    bpmLabel.setVisible (tempoSyncOn);
    resized();
}

// =====================================================================================
// Coordinate conversion
// =====================================================================================

juce::Rectangle<float> LFOComponent::getGraphBounds() const
{
    const float controlSpace = hasRateSlider ? kSliderHeight * 2.0f : 0.0f;
    return { kMargin + kShapeStripWidth, kMargin,
        static_cast<float> (getWidth()) - 2.0f * kMargin - kShapeStripWidth,
        static_cast<float> (getHeight()) - controlSpace - 2.0f * kMargin };
}

float LFOComponent::positionToX (float position) const
{
    auto bounds = getGraphBounds();
    return bounds.getX() + position * bounds.getWidth();
}

float LFOComponent::valueToY (float value) const
{
    auto bounds = getGraphBounds();
    return bounds.getY() + (1.0f - value) * bounds.getHeight();
}

float LFOComponent::xToPosition (float x) const
{
    auto bounds = getGraphBounds();
    return juce::jlimit (0.0f, 1.0f, (x - bounds.getX()) / bounds.getWidth());
}

float LFOComponent::yToValue (float y) const
{
    auto bounds = getGraphBounds();
    return juce::jlimit (0.0f, 1.0f, 1.0f - (y - bounds.getY()) / bounds.getHeight());
}

// =====================================================================================
// Hit testing
// =====================================================================================

int LFOComponent::hitTestPoint (float x, float y) const
{
    auto points = lfoData->getPoints();
    for (int i = 0; i < static_cast<int> (points.size()); ++i)
    {
        float px = positionToX (points[i].position);
        float py = valueToY (points[i].value);
        float dx = x - px;
        float dy = y - py;
        if (dx * dx + dy * dy <= kHitRadius * kHitRadius)
            return i;
    }
    return -1;
}

juce::Point<float> LFOComponent::getCurveHandlePosition (size_t segmentIndex) const
{
    auto points = lfoData->getPoints();
    if (segmentIndex >= points.size() - 1)
        return {};

    const auto& p1 = points[segmentIndex];
    const auto& p2 = points[segmentIndex + 1];

    float midPos = (p1.position + p2.position) * 0.5f;
    float midVal = lfoData->getValueAt (midPos);

    return { positionToX (midPos), valueToY (midVal) };
}

int LFOComponent::hitTestCurveHandle (float x, float y) const
{
    auto points = lfoData->getPoints();
    if (points.size() < 2)
        return -1;

    for (int i = 0; i < static_cast<int> (points.size()) - 1; ++i)
    {
        auto handlePos = getCurveHandlePosition (static_cast<size_t> (i));
        float dx = x - handlePos.x;
        float dy = y - handlePos.y;
        if (dx * dx + dy * dy <= kHitRadius * kHitRadius)
            return i;
    }
    return -1;
}

// =====================================================================================
// Paint
// =====================================================================================

void LFOComponent::paint (juce::Graphics& g)
{
    if (!lfoData)
        return;

    auto graphBounds = getGraphBounds();

    // Background
    g.setColour (SECONDARY_COLOR);
    g.fillRect (graphBounds);

    // Grid lines
    g.setColour (juce::Colours::grey.withAlpha (0.2f));
    for (float p : { 0.25f, 0.5f, 0.75f })
    {
        float x = positionToX (p);
        g.drawLine (x, graphBounds.getY(), x, graphBounds.getBottom(), 0.5f);
    }
    float midY = valueToY (0.5f);
    g.drawLine (graphBounds.getX(), midY, graphBounds.getRight(), midY, 0.5f);

    auto points = lfoData->getPoints();
    if (points.empty())
        return;

    // Waveform path
    juce::Path path;
    bool pathStarted = false;
    for (float t = 0.0f; t <= 1.0f; t += 0.005f)
    {
        float value = lfoData->getValueAt (t);
        float x = positionToX (t);
        float y = valueToY (value);

        if (!pathStarted)
        {
            path.startNewSubPath (x, y);
            pathStarted = true;
        }
        else
        {
            path.lineTo (x, y);
        }
    }
    g.setColour (juce::Colours::white);
    g.strokePath (path, juce::PathStrokeType (2.0f));

    // Curve handles
    for (int i = 0; i < static_cast<int> (points.size()) - 1; ++i)
    {
        auto handlePos = getCurveHandlePosition (static_cast<size_t> (i));
        bool selected = (dragTarget == DragTarget::CurveHandle && selectedCurveIndex == i);
        g.setColour (selected ? juce::Colours::cyan : juce::Colours::grey);
        g.fillEllipse (handlePos.x - kCurveHandleRadius, handlePos.y - kCurveHandleRadius,
            kCurveHandleRadius * 2.0f, kCurveHandleRadius * 2.0f);
    }

    // Control points
    for (int i = 0; i < static_cast<int> (points.size()); ++i)
    {
        float px = positionToX (points[i].position);
        float py = valueToY (points[i].value);
        bool selected = (dragTarget == DragTarget::Point && selectedPointIndex == i);
        bool isBoundary = (i == 0 || i == static_cast<int> (points.size()) - 1);

        // Boundary indicator ring
        if (isBoundary)
        {
            g.setColour (juce::Colours::orange.withAlpha (0.4f));
            g.drawEllipse (px - kPointRadius - 2.0f, py - kPointRadius - 2.0f,
                (kPointRadius + 2.0f) * 2.0f, (kPointRadius + 2.0f) * 2.0f, 1.5f);
        }

        g.setColour (selected ? juce::Colours::yellow : juce::Colours::orange);
        g.fillEllipse (px - kPointRadius, py - kPointRadius,
            kPointRadius * 2.0f, kPointRadius * 2.0f);
    }
}

// =====================================================================================
// Layout
// =====================================================================================

void LFOComponent::resized()
{
    // Layout shape preset + flip buttons vertically centered to the left of the graph
    {
        auto graphBounds = getGraphBounds();
        const float gap = 4.0f;
        const int numButtons = 6;
        const float totalHeight = kShapeButtonSize * static_cast<float> (numButtons) + gap * static_cast<float> (numButtons - 1);
        const float startY = graphBounds.getY() + (graphBounds.getHeight() - totalHeight) * 0.5f;
        const float btnX = kMargin + (kShapeStripWidth - kShapeButtonSize) * 0.5f;

        ShapePresetButton* buttons[] = { &sineButton, &triangleButton, &sawButton, &squareButton, &flipHButton, &flipVButton };
        for (int i = 0; i < numButtons; ++i)
        {
            buttons[i]->setBounds (static_cast<int> (btnX),
                static_cast<int> (startY + static_cast<float> (i) * (kShapeButtonSize + gap)),
                static_cast<int> (kShapeButtonSize),
                static_cast<int> (kShapeButtonSize));
        }
    }

    if (hasRateSlider)
    {
        auto area = getLocalBounds();

        // Bottom row 2: rate slider OR note division + beat lock
        auto row2 = area.removeFromBottom (static_cast<int> (kSliderHeight));

        if (rateSlider.isVisible())
        {
            // Free Hz mode: [Rate label] [slider]
            row2.removeFromLeft (40); // space for "Rate" label
            rateSlider.setBounds (row2);
        }
        else
        {
            // Tempo sync mode: [note division combo] [beat lock toggle]
            noteDivisionCombo.setBounds (row2.removeFromLeft (row2.getWidth() / 2));
            beatSyncToggle.setBounds (row2);
        }

        // Bottom row 1: sync toggle + BPM slider (when synced)
        auto row1 = area.removeFromBottom (static_cast<int> (kSliderHeight));
        tempoSyncToggle.setBounds (row1.removeFromLeft (70));

        if (bpmSlider.isVisible())
        {
            row1.removeFromLeft (35); // space for "BPM" label
            bpmSlider.setBounds (row1);
        }
    }
}

// =====================================================================================
// Mouse interaction
// =====================================================================================

void LFOComponent::mouseDown (const juce::MouseEvent& e)
{
    float mx = static_cast<float> (e.x);
    float my = static_cast<float> (e.y);

    // Hit-test points first
    int pointIdx = hitTestPoint (mx, my);
    if (pointIdx >= 0)
    {
        dragTarget = DragTarget::Point;
        selectedPointIndex = pointIdx;
        selectedCurveIndex = -1;
        isStuck = false;
        stickAccumulator = 0.0f;
        repaint();
        return;
    }

    // Hit-test curve handles
    int curveIdx = hitTestCurveHandle (mx, my);
    if (curveIdx >= 0)
    {
        dragTarget = DragTarget::CurveHandle;
        selectedCurveIndex = curveIdx;
        selectedPointIndex = -1;
        repaint();
        return;
    }

    // No hit
    dragTarget = DragTarget::None;
    selectedPointIndex = -1;
    selectedCurveIndex = -1;
    repaint();
}

void LFOComponent::mouseDoubleClick (const juce::MouseEvent& e)
{
    float mx = static_cast<float> (e.x);
    float my = static_cast<float> (e.y);

    // Check if clicking near an existing interior point — remove it
    int pointIdx = hitTestPoint (mx, my);
    if (pointIdx > 0 && pointIdx < static_cast<int> (lfoData->getNumPoints()) - 1)
    {
        lfoData->removePoint (static_cast<size_t> (pointIdx));
        selectedPointIndex = -1;
        dragTarget = DragTarget::None;
        return;
    }

    // Clicking empty space — add a new point
    auto graphBounds = getGraphBounds();
    if (graphBounds.contains (mx, my))
    {
        float pos = xToPosition (mx);
        float val = yToValue (my);
        lfoData->addPoint (pos, val);
    }
}

void LFOComponent::mouseDrag (const juce::MouseEvent& e)
{
    float mx = static_cast<float> (e.x);
    float my = static_cast<float> (e.y);

    if (dragTarget == DragTarget::Point && selectedPointIndex >= 0)
    {
        auto points = lfoData->getPoints();
        int numPoints = static_cast<int> (points.size());
        if (selectedPointIndex >= numPoints)
            return;

        bool isBoundary = (selectedPointIndex == 0 || selectedPointIndex == numPoints - 1);

        if (isBoundary)
        {
            // Boundary points: Y only, linked
            float newValue = yToValue (my);
            lfoData->updatePointValue (0, newValue);
            lfoData->updatePointValue (static_cast<size_t> (numPoints - 1), newValue);
        }
        else
        {
            // Interior points: X + Y with sticky behavior
            float newPos = xToPosition (mx);
            float newValue = yToValue (my);

            // Get neighbor positions for clamping
            float leftNeighborPos = points[selectedPointIndex - 1].position;
            float rightNeighborPos = points[selectedPointIndex + 1].position;

            if (isStuck)
            {
                // Accumulate drag distance
                float pixelDelta = mx - positionToX (points[selectedPointIndex].position);
                stickAccumulator += std::abs (pixelDelta);

                if (stickAccumulator > kStickThreshold)
                {
                    // Unstick — allow leapfrog
                    isStuck = false;
                    stickAccumulator = 0.0f;
                }
                else
                {
                    // Stay stuck — don't move X
                    newPos = points[selectedPointIndex].position;
                }
            }

            if (!isStuck)
            {
                // Check if approaching a neighbor — snap
                float leftSnapX = positionToX (leftNeighborPos);
                float rightSnapX = positionToX (rightNeighborPos);
                constexpr float snapPixels = 5.0f;

                if (std::abs (mx - leftSnapX) < snapPixels && selectedPointIndex > 1)
                {
                    newPos = leftNeighborPos + 0.001f;
                    isStuck = true;
                    stickAccumulator = 0.0f;
                }
                else if (std::abs (mx - rightSnapX) < snapPixels && selectedPointIndex < numPoints - 2)
                {
                    newPos = rightNeighborPos - 0.001f;
                    isStuck = true;
                    stickAccumulator = 0.0f;
                }
                else
                {
                    // Normal clamping between neighbors
                    newPos = juce::jlimit (leftNeighborPos + 0.001f, rightNeighborPos - 0.001f, newPos);
                }
            }

            float currentCurve = points[selectedPointIndex].curve;
            lfoData->updatePoint (static_cast<size_t> (selectedPointIndex), newPos, newValue, currentCurve);

            // After sort, our point may have moved index. Find it.
            auto updatedPoints = lfoData->getPoints();
            for (int i = 0; i < static_cast<int> (updatedPoints.size()); ++i)
            {
                if (std::abs (updatedPoints[i].position - newPos) < 0.0001f
                    && std::abs (updatedPoints[i].value - newValue) < 0.0001f)
                {
                    selectedPointIndex = i;
                    break;
                }
            }
        }
    }
    else if (dragTarget == DragTarget::CurveHandle && selectedCurveIndex >= 0)
    {
        auto points = lfoData->getPoints();
        if (selectedCurveIndex >= static_cast<int> (points.size()) - 1)
            return;

        // Map vertical displacement to curve value
        const auto& p1 = points[selectedCurveIndex];
        const auto& p2 = points[selectedCurveIndex + 1];
        float midPos = (p1.position + p2.position) * 0.5f;
        float linearMidValue = (p1.value + p2.value) * 0.5f;
        float linearMidY = valueToY (linearMidValue);

        // Displacement from the linear midpoint
        float displacement = (linearMidY - my) / (getGraphBounds().getHeight() * 0.5f);

        // Flip for ascending segments so dragging up always bows the curve upward
        if (p2.value > p1.value)
            displacement = -displacement;

        float newCurve = juce::jlimit (-1.0f, 1.0f, displacement);

        lfoData->updateCurve (static_cast<size_t> (selectedCurveIndex), newCurve);
    }
}

void LFOComponent::mouseUp (const juce::MouseEvent&)
{
    dragTarget = DragTarget::None;
    selectedPointIndex = -1;
    selectedCurveIndex = -1;
    isStuck = false;
    stickAccumulator = 0.0f;
    repaint();
}

void LFOComponent::mouseMove (const juce::MouseEvent& e)
{
    float mx = static_cast<float> (e.x);
    float my = static_cast<float> (e.y);

    if (hitTestPoint (mx, my) >= 0 || hitTestCurveHandle (mx, my) >= 0)
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    else
        setMouseCursor (juce::MouseCursor::NormalCursor);
}
