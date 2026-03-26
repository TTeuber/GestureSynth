#include "LFOComponent.h"
#include <algorithm>

LFOComponent::LFOComponent (std::shared_ptr<LFOData> data, juce::AudioProcessorValueTreeState& parameters, bool showRateSlider, int lfoIdx,
                            std::shared_ptr<MyLFO*> lfoPtr, AnimationFrameSource* animSrc)
    : lfoData (std::move (data)),
      apvts (parameters),
      hasRateSlider (showRateSlider),
      lfoIndex (lfoIdx),
      myLFO (std::move (lfoPtr)),
      animSource (animSrc),
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
        auto* rateParam = parameters.getParameter (paramId ("Rate", lfoIndex));
        auto* noteDivParam = dynamic_cast<juce::AudioParameterChoice*> (parameters.getParameter (paramId ("NoteDivision", lfoIndex)));
        auto* tempoSyncParam = dynamic_cast<juce::AudioParameterBool*> (parameters.getParameter (paramId ("TempoSync", lfoIndex)));
        auto* beatSyncParam = dynamic_cast<juce::AudioParameterBool*> (parameters.getParameter (paramId ("BeatSync", lfoIndex)));
        auto* monoParam = dynamic_cast<juce::AudioParameterBool*> (parameters.getParameter (paramId ("Mono", lfoIndex)));

        rateComponent = std::make_unique<LFORateComponent> (rateParam, noteDivParam, tempoSyncParam);
        addAndMakeVisible (*rateComponent);

        bpmToggle = std::make_unique<CustomToggleComponent> (tempoSyncParam, "BPM");
        addAndMakeVisible (*bpmToggle);

        hostToggle = std::make_unique<CustomToggleComponent> (beatSyncParam, "Host");
        hostToggle->setOpacityCallback ([tempoSyncParam]() { return tempoSyncParam->get(); });
        addAndMakeVisible (*hostToggle);

        monoToggle = std::make_unique<CustomToggleComponent> (monoParam, "Mono");
        addAndMakeVisible (*monoToggle);
    }

    addAndMakeVisible (sineButton);
    addAndMakeVisible (triangleButton);
    addAndMakeVisible (sawButton);
    addAndMakeVisible (squareButton);
    addAndMakeVisible (flipHButton);
    addAndMakeVisible (flipVButton);

    if (animSource != nullptr)
        animSource->addListener (this, AnimationFrameSource::Rate::Hz60);

    setSize (300, 200);
}

LFOComponent::~LFOComponent()
{
    if (animSource != nullptr)
        animSource->removeListener (this);
    lfoData->removeListener (this);
}

void LFOComponent::rebind (std::shared_ptr<LFOData> newData, int newLfoIndex, std::shared_ptr<MyLFO*> newLfoPtr)
{
    lfoData->removeListener (this);

    lfoData = std::move (newData);
    lfoIndex = newLfoIndex;
    myLFO = std::move (newLfoPtr);

    lfoData->addListener (this);

    if (hasRateSlider)
    {
        auto* rateParam = apvts.getParameter (paramId ("Rate", lfoIndex));
        auto* noteDivParam = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (paramId ("NoteDivision", lfoIndex)));
        auto* tempoSyncParam = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (paramId ("TempoSync", lfoIndex)));
        auto* beatSyncParam = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (paramId ("BeatSync", lfoIndex)));
        auto* monoParam = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (paramId ("Mono", lfoIndex)));

        rateComponent->rebind (rateParam, noteDivParam, tempoSyncParam);
        bpmToggle->rebind (tempoSyncParam);
        hostToggle->rebind (beatSyncParam);
        hostToggle->setOpacityCallback ([tempoSyncParam]() { return tempoSyncParam->get(); });
        monoToggle->rebind (monoParam);
    }

    repaint();
}

void LFOComponent::lfoDataChanged()
{
    repaint();
}

void LFOComponent::onAnimationFrame()
{
    if (myLFO != nullptr && *myLFO != nullptr)
    {
        const float phase = (*myLFO)->getPhase();
        if (phase != currentPhase || !showPhaseIndicator)
        {
            currentPhase = phase;
            showPhaseIndicator = true;
            repaint();
        }
    }
    else if (showPhaseIndicator)
    {
        showPhaseIndicator = false;
        repaint();
    }
}

// =====================================================================================
// Coordinate conversion
// =====================================================================================

juce::Rectangle<float> LFOComponent::getGraphBounds() const
{
    const float controlSpace = hasRateSlider ? kControlColumnWidth : 0.0f;
    return { kMargin + kShapeStripWidth, kMargin,
        static_cast<float> (getWidth()) - 2.0f * kMargin - kShapeStripWidth - controlSpace,
        static_cast<float> (getHeight()) - 2.0f * kMargin };
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
        if (hoveredCurveIndex == i)
        {
            g.setColour (juce::Colours::grey.withAlpha (0.3f));
            g.fillEllipse (handlePos.x - kCurveHandleRadius * 2.0f, handlePos.y - kCurveHandleRadius * 2.0f,
                kCurveHandleRadius * 4.0f, kCurveHandleRadius * 4.0f);
        }
        g.setColour (juce::Colours::grey);
        g.fillEllipse (handlePos.x - kCurveHandleRadius, handlePos.y - kCurveHandleRadius,
            kCurveHandleRadius * 2.0f, kCurveHandleRadius * 2.0f);
    }

    // Control points
    for (int i = 0; i < static_cast<int> (points.size()); ++i)
    {
        float px = positionToX (points[i].position);
        float py = valueToY (points[i].value);
        bool isBoundary = (i == 0 || i == static_cast<int> (points.size()) - 1);

        // Boundary indicator ring
        if (isBoundary)
        {
            g.setColour (juce::Colours::white.withAlpha (0.4f));
            g.drawEllipse (px - kPointRadius - 2.0f, py - kPointRadius - 2.0f,
                (kPointRadius + 2.0f) * 2.0f, (kPointRadius + 2.0f) * 2.0f, 1.5f);
        }

        if (hoveredPointIndex == i)
        {
            g.setColour (juce::Colours::white.withAlpha (0.3f));
            g.fillEllipse (px - kPointRadius * 2.0f, py - kPointRadius * 2.0f,
                kPointRadius * 4.0f, kPointRadius * 4.0f);
        }
        g.setColour (juce::Colours::white);
        g.fillEllipse (px - kPointRadius, py - kPointRadius,
            kPointRadius * 2.0f, kPointRadius * 2.0f);
    }

    // Phase indicator vertical line
    if (showPhaseIndicator)
    {
        const float phaseX = positionToX (currentPhase);
        g.setColour (juce::Colours::white.withAlpha (0.6f));
        g.drawLine (phaseX, graphBounds.getY(), phaseX, graphBounds.getBottom(), 1.5f);
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
        const float startY = graphBounds.getY();
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
        auto controlCol = area.removeFromRight (static_cast<int> (kControlColumnWidth));
        controlCol.removeFromTop (static_cast<int> (kMargin));
        controlCol.removeFromBottom (static_cast<int> (kMargin));

        int controlHeight = controlCol.getHeight() / 4;
        const int pad = 2;

        monoToggle->setBounds (controlCol.removeFromTop (controlHeight).reduced (pad));
        hostToggle->setBounds (controlCol.removeFromTop (controlHeight).reduced (pad));
        bpmToggle->setBounds (controlCol.removeFromTop (controlHeight).reduced (pad));
        rateComponent->setBounds (controlCol.reduced (pad));
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

        const auto& p1 = points[selectedCurveIndex];
        const auto& p2 = points[selectedCurveIndex + 1];

        float mouseValue = yToValue (my);
        float range = p2.value - p1.value;

        float newCurve;
        if (std::abs (range) < 0.001f)
        {
            newCurve = 0.0f;
        }
        else
        {
            float t = juce::jlimit (0.001f, 0.999f, (mouseValue - p1.value) / range);
            newCurve = juce::jlimit (-1.0f, 1.0f, CurveUtils::inverseCurveAtHalf (t));
        }

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

    int newHoveredPoint = hitTestPoint (mx, my);
    int newHoveredCurve = newHoveredPoint < 0 ? hitTestCurveHandle (mx, my) : -1;

    if (newHoveredPoint != hoveredPointIndex || newHoveredCurve != hoveredCurveIndex)
    {
        hoveredPointIndex = newHoveredPoint;
        hoveredCurveIndex = newHoveredCurve;
        repaint();
    }

    if (hoveredPointIndex >= 0 || hoveredCurveIndex >= 0)
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    else
        setMouseCursor (juce::MouseCursor::NormalCursor);
}

void LFOComponent::mouseExit (const juce::MouseEvent&)
{
    if (hoveredPointIndex >= 0 || hoveredCurveIndex >= 0)
    {
        hoveredPointIndex = -1;
        hoveredCurveIndex = -1;
        repaint();
    }
}
