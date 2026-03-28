#include "ADSRGraph.h"

#include <utility>

// clang-format off
ADSRGraph::ADSRGraph(juce::AudioProcessorValueTreeState& p,
    const juce::StringRef attackParam,
    const juce::StringRef attackCurveParam,
    const juce::StringRef decayParam,
    const juce::StringRef decayCurveParam,
    const juce::StringRef sustainParam,
    const juce::StringRef releaseParam,
    const juce::StringRef releaseCurveParam,
    std::shared_ptr<MyADSR*> adsr,
    juce::UndoManager* undoManager,
    std::atomic<int>* gestureCount,
    AnimationFrameSource* animSource)
    : parameters(p),
    attackId(attackParam),
    attackCurveId (attackCurveParam),
    decayId(decayParam),
    decayCurveId (decayCurveParam),
    sustainId(sustainParam),
    releaseId(releaseParam),
    releaseCurveId (releaseCurveParam),
    myADSR(std::move(adsr)),
    undoManager(undoManager),
    gestureCount(gestureCount),
    animSource(animSource)
// clang-format on
{
    parameters.addParameterListener (attackId, this);
    parameters.addParameterListener (attackCurveId, this);
    parameters.addParameterListener (decayId, this);
    parameters.addParameterListener (decayCurveId, this);
    parameters.addParameterListener (sustainId, this);
    parameters.addParameterListener (releaseId, this);
    parameters.addParameterListener (releaseCurveId, this);

    selectedPoint = None;

    attackTime = parameters.getRawParameterValue (attackId)->load();
    attackCurve = parameters.getRawParameterValue (attackCurveId)->load();
    decayTime = parameters.getRawParameterValue (decayId)->load();
    decayCurve = parameters.getRawParameterValue (decayCurveId)->load();
    sustainLevel = (1 - parameters.getRawParameterValue (sustainId)->load()) * static_cast<float> (getHeight());
    releaseTime = parameters.getRawParameterValue (releaseId)->load();
    releaseCurve = parameters.getRawParameterValue (releaseCurveId)->load();

    totalDuration = attackTime + decayTime + releaseTime;

    if (animSource != nullptr)
        animSource->addListener (this, AnimationFrameSource::Rate::Hz60);
}

ADSRGraph::~ADSRGraph()
{
    if (animSource != nullptr)
        animSource->removeListener (this);
    parameters.removeParameterListener (attackId, this);
    parameters.removeParameterListener (attackCurveId, this);
    parameters.removeParameterListener (decayId, this);
    parameters.removeParameterListener (decayCurveId, this);
    parameters.removeParameterListener (sustainId, this);
    parameters.removeParameterListener (releaseId, this);
    parameters.removeParameterListener (releaseCurveId, this);
}

void ADSRGraph::rebind (
    const juce::StringRef newAttackParam,
    const juce::StringRef newAttackCurveParam,
    const juce::StringRef newDecayParam,
    const juce::StringRef newDecayCurveParam,
    const juce::StringRef newSustainParam,
    const juce::StringRef newReleaseParam,
    const juce::StringRef newReleaseCurveParam,
    std::shared_ptr<MyADSR*> adsr)
{
    // Remove old parameter listeners
    parameters.removeParameterListener (attackId, this);
    parameters.removeParameterListener (attackCurveId, this);
    parameters.removeParameterListener (decayId, this);
    parameters.removeParameterListener (decayCurveId, this);
    parameters.removeParameterListener (sustainId, this);
    parameters.removeParameterListener (releaseId, this);
    parameters.removeParameterListener (releaseCurveId, this);

    // Update stored IDs
    attackId = newAttackParam;
    attackCurveId = newAttackCurveParam;
    decayId = newDecayParam;
    decayCurveId = newDecayCurveParam;
    sustainId = newSustainParam;
    releaseId = newReleaseParam;
    releaseCurveId = newReleaseCurveParam;

    // Update ADSR pointer
    myADSR = std::move (adsr);

    // Add new parameter listeners
    parameters.addParameterListener (attackId, this);
    parameters.addParameterListener (attackCurveId, this);
    parameters.addParameterListener (decayId, this);
    parameters.addParameterListener (decayCurveId, this);
    parameters.addParameterListener (sustainId, this);
    parameters.addParameterListener (releaseId, this);
    parameters.addParameterListener (releaseCurveId, this);

    // Read current parameter values
    attackTime = parameters.getRawParameterValue (attackId)->load();
    attackCurve = parameters.getRawParameterValue (attackCurveId)->load();
    decayTime = parameters.getRawParameterValue (decayId)->load();
    decayCurve = parameters.getRawParameterValue (decayCurveId)->load();
    sustainLevel = (1 - parameters.getRawParameterValue (sustainId)->load()) * height;
    releaseTime = parameters.getRawParameterValue (releaseId)->load();
    releaseCurve = parameters.getRawParameterValue (releaseCurveId)->load();

    totalDuration = attackTime + decayTime + releaseTime;

    repaint();
}

void ADSRGraph::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == attackId)
        attackTime = juce::jmax<float> (0.01, newValue);
    else if (parameterID == attackCurveId)
        attackCurve = newValue;
    else if (parameterID == decayId)
        decayTime = juce::jmax<float> (0.01, newValue);
    else if (parameterID == decayCurveId)
        decayCurve = newValue;
    else if (parameterID == sustainId)
        sustainLevel = (1 - newValue) * height;
    else if (parameterID == releaseId)
        releaseTime = juce::jmax<float> (0.01, newValue);
    else if (parameterID == releaseCurveId)
        releaseCurve = newValue;

    repaint();
}

void ADSRGraph::setParameters (const float attack, const float decay, const float sustain, const float release)
{
    attackTime = juce::jmax<float> (0.01, attack);
    decayTime = decay;
    sustainLevel = (1 - sustain) * height;
    releaseTime = release;
    repaint();
}

void ADSRGraph::resized()
{
    auto innerBoxBounds = getLocalBounds().reduced (4).reduced (4);
    innerBoxOrigin = innerBoxBounds.getPosition();
    width = static_cast<float> (innerBoxBounds.getWidth());
    height = static_cast<float> (innerBoxBounds.getHeight());
    sustainLevel = (1 - parameters.getRawParameterValue (sustainId)->load()) * height;
}

void ADSRGraph::paint (juce::Graphics& g)
{
    // Fill with parent background
    g.fillAll (PRIMARY_COLOR);

    // Outer box
    auto outerBounds = getLocalBounds();
    PaintHelpers::drawComponentBox (g, outerBounds.toFloat());

    // Inner box area — padding matches DualParameterComponent (4 + 4 = 8px per side)
    auto innerArea = outerBounds.reduced (4);
    auto innerBoxBounds = innerArea.reduced (4);
    PaintHelpers::drawInnerBox (g, innerBoxBounds.toFloat());

    // Clip and translate into the inner box for all ADSR drawing
    g.reduceClipRegion (innerBoxBounds);
    g.setOrigin (innerBoxBounds.getPosition());

    const float w = static_cast<float> (innerBoxBounds.getWidth());
    const float h = static_cast<float> (innerBoxBounds.getHeight());

    g.setColour (TEXT_COLOR);

    attackX = attackTime / durationWidth * w - xOffset;
    attackPoint = { attackX, 0 };

    decayX = (attackTime + decayTime) / durationWidth * w - xOffset;
    decayPoint = { decayX, sustainLevel };

    releaseX = (attackTime + decayTime + releaseTime) / durationWidth * w - xOffset;
    releasePoint = { releaseX, (h) };

    attackCurvePoint = { attackX / 2, getCurveY (attackX / 2) };

    float decayCurveX = (decayX - attackX) / 2 + attackX;
    decayCurvePoint = { decayCurveX, getCurveY (decayCurveX) };

    float releaseCurveX = (releaseX - decayX) / 2 + decayX;
    releaseCurvePoint = { releaseCurveX, getCurveY (releaseCurveX) };

    // Main control points (matching LFO style: radius 5, white)
    constexpr float kPointRadius = 5.0f;
    constexpr float kCurveHandleRadius = 4.0f;

    auto drawMainPoint = [&] (float x, float y, Point which) {
        if (hoveredPoint == which)
        {
            g.setColour (juce::Colours::white.withAlpha (0.3f));
            g.fillEllipse (x - kPointRadius * 2.0f, y - kPointRadius * 2.0f,
                kPointRadius * 4.0f, kPointRadius * 4.0f);
        }
        g.setColour (juce::Colours::white);
        g.fillEllipse (x - kPointRadius, y - kPointRadius,
            kPointRadius * 2.0f, kPointRadius * 2.0f);
    };

    drawMainPoint (attackX, attackPoint.getY(), Attack);
    drawMainPoint (decayX, decayPoint.getY(), Decay);
    drawMainPoint (releaseX, releasePoint.getY(), Release);

    // Curve handles (matching LFO style: radius 4, grey)
    auto drawCurveHandle = [&] (float x, float y, Point which) {
        if (hoveredPoint == which)
        {
            g.setColour (juce::Colours::grey.withAlpha (0.3f));
            g.fillEllipse (x - kCurveHandleRadius * 2.0f, y - kCurveHandleRadius * 2.0f,
                kCurveHandleRadius * 4.0f, kCurveHandleRadius * 4.0f);
        }
        g.setColour (juce::Colours::grey);
        g.fillEllipse (x - kCurveHandleRadius, y - kCurveHandleRadius,
            kCurveHandleRadius * 2.0f, kCurveHandleRadius * 2.0f);
    };

    drawCurveHandle (attackCurvePoint.x, attackCurvePoint.y, AttackCurve);
    drawCurveHandle (decayCurvePoint.x, decayCurvePoint.y, DecayCurve);
    drawCurveHandle (releaseCurvePoint.x, releaseCurvePoint.y, ReleaseCurve);

    if (showTimePoint)
    {
        g.setColour (juce::Colours::white);
        g.fillEllipse (timePoint.getX() - kCurveHandleRadius, timePoint.getY() - kCurveHandleRadius,
            kCurveHandleRadius * 2.0f, kCurveHandleRadius * 2.0f);
    }

    juce::Path adsrPath;

    drawCurve (g);

    for (float i = 0; i < durationWidth * 3; i += 1.0f)
    {
        const auto markerPosition = w / durationWidth * (i + 1);
        const auto secondNumber = static_cast<int> (i + 1);
        const bool isOdd = (secondNumber % 2 != 0);
        const bool subdivisionsVisible = durationWidth < 6.0f;

        if (! subdivisionsVisible && isOdd)
        {
            g.setColour (juce::Colours::grey.withAlpha (0.15f));
            juce::Path marker;
            marker.startNewSubPath (markerPosition - xOffset, 0);
            marker.lineTo (markerPosition - xOffset, h);
            g.strokePath (marker, juce::PathStrokeType (1.0f));
        }
        else
        {
            g.setColour (juce::Colours::grey.withAlpha (0.3f));
            juce::Path marker;
            marker.startNewSubPath (markerPosition - xOffset, 0);
            marker.lineTo (markerPosition - xOffset, h);
            g.strokePath (marker, juce::PathStrokeType (2.0f));
            g.drawText (juce::String (secondNumber), markerPosition - (durationWidth / 10) - xOffset, h - 20, 20, 20, juce::Justification::centred);
        }

        if (subdivisionsVisible)
        {
            g.setColour (juce::Colours::grey.withAlpha (0.15f));
            juce::Path subMarker;
            subMarker.startNewSubPath (markerPosition - xOffset - w / durationWidth / 2, 0);
            subMarker.lineTo (markerPosition - xOffset - w / durationWidth / 2, h);
            g.strokePath (subMarker, juce::PathStrokeType (1.0f));
        }
    }

    g.strokePath (adsrPath, juce::PathStrokeType (4.0f));
}

void ADSRGraph::drawCurve (juce::Graphics& g) const
{
    juce::Path path;

    constexpr auto startX = 0.0f;
    constexpr auto step = 2.0f;

    // Build a sorted list of x values to sample, including critical transition points
    std::vector<float> xValues;
    for (float x = startX; x <= releaseX; x += step)
        xValues.push_back (x);

    for (const auto cp : { attackX, decayX, releaseX })
    {
        if (cp >= startX && cp <= releaseX)
            xValues.push_back (cp);
    }

    std::sort (xValues.begin(), xValues.end());
    xValues.erase (std::unique (xValues.begin(), xValues.end()), xValues.end());

    bool firstPoint = true;
    for (const float x : xValues)
    {
        const float y = getCurveY (x);

        if (firstPoint)
        {
            path.startNewSubPath (x, y);
            firstPoint = false;
        }
        else
        {
            path.lineTo (x, y);
        }
    }

    path.lineTo (releaseX, height);

    g.setColour (juce::Colours::white);
    g.strokePath (path, juce::PathStrokeType (2.0f));
}

float ADSRGraph::getCurveY (const float x) const
{
    if (x < attackX)
        return (1 - MyADSR::toAttackCurve (x / attackTime / width * durationWidth, attackCurve)) * height;

    if (x < decayX)
        return (1 - MyADSR::toDecayCurve ((x - attackX) / juce::jmax<float> (0.0001, decayTime) / width * durationWidth, 1 - sustainLevel / height, decayCurve)) * height;

    if (x < releaseX)
        return (1 - MyADSR::toReleaseCurve (juce::jmin<float> (1, (x - decayX) / juce::jmax<float> (0.0001, releaseTime) / width * durationWidth), 1 - sustainLevel / height, releaseCurve)) * height;

    return height;
}

void ADSRGraph::showTime()
{
    if (myADSR != nullptr && *myADSR != nullptr && (*myADSR)->isActive())
    {
        const std::array<float, 2> xy = (*myADSR)->getTimePoint();
        const float x = xy[0] / durationWidth * width - xOffset;
        timePoint = { x, getCurveY (x) };
        showTimePoint = true;
        repaint();
    }
    else if (showTimePoint)
    {
        showTimePoint = false;
        repaint();
    }
}

void ADSRGraph::onAnimationFrame()
{
    showTime();
}

void ADSRGraph::mouseMove (const juce::MouseEvent& event)
{
    auto mousePos = juce::Point (event.position.x - innerBoxOrigin.x, event.position.y - innerBoxOrigin.y);
    constexpr Point pointEnums[] = { Attack, AttackCurve, Decay, DecayCurve, Release, ReleaseCurve };
    Point newHovered = None;
    for (int i = 0; i < static_cast<int> (points.size()); ++i)
    {
        if (points[i]->getDistanceFrom (mousePos) < 20)
        {
            newHovered = pointEnums[i];
            break;
        }
    }

    if (newHovered != hoveredPoint)
    {
        hoveredPoint = newHovered;
        repaint();
    }

    if (hoveredPoint != None)
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    else
        setMouseCursor (juce::MouseCursor::NormalCursor);
}

void ADSRGraph::mouseExit (const juce::MouseEvent&)
{
    if (hoveredPoint != None)
    {
        hoveredPoint = None;
        repaint();
    }
}

void ADSRGraph::mouseDown (const juce::MouseEvent& event)
{
    clickPoint = { event.position.x - innerBoxOrigin.x, event.position.y - innerBoxOrigin.y };
    constexpr float clickWidth = 20;

    if (clickPoint.getDistanceFrom (attackPoint) < clickWidth)
        selectedPoint = Attack;
    else if (clickPoint.getDistanceFrom (attackCurvePoint) < clickWidth)
        selectedPoint = AttackCurve;
    else if (clickPoint.getDistanceFrom (decayPoint) < clickWidth)
        selectedPoint = Decay;
    else if (clickPoint.getDistanceFrom (decayCurvePoint) < clickWidth)
        selectedPoint = DecayCurve;
    else if (clickPoint.getDistanceFrom (releasePoint) < clickWidth)
        selectedPoint = Release;
    else if (clickPoint.getDistanceFrom (releaseCurvePoint) < clickWidth)
        selectedPoint = ReleaseCurve;
    else
        selectedPoint = None;

    if (selectedPoint != None)
    {
        if (undoManager != nullptr)
            undoManager->beginNewTransaction();
        beginGesturesForSelectedPoint();
        if (gestureCount != nullptr)
            ++(*gestureCount);
    }
}

void ADSRGraph::mouseUp (const juce::MouseEvent& event)
{
    if (!activeGestureParams.empty())
    {
        for (auto* p : activeGestureParams)
            p->endChangeGesture();
        activeGestureParams.clear();
        if (gestureCount != nullptr)
            --(*gestureCount);
    }
    selectedPoint = None;
}

void ADSRGraph::beginGesturesForSelectedPoint()
{
    activeGestureParams.clear();
    auto addParam = [this] (const juce::String& id) {
        if (auto* p = parameters.getParameter (id))
        {
            p->beginChangeGesture();
            activeGestureParams.push_back (p);
        }
    };

    switch (selectedPoint)
    {
        case Attack:       addParam (attackId); break;
        case AttackCurve:  addParam (attackCurveId); break;
        case Decay:        addParam (decayId); addParam (sustainId); break;
        case DecayCurve:   addParam (decayCurveId); break;
        case Release:      addParam (releaseId); break;
        case ReleaseCurve: addParam (releaseCurveId); break;
        default: break;
    }
}

void ADSRGraph::mouseDrag (const juce::MouseEvent& event)
{
    // Translate mouse position into inner-box-local coordinates
    const float mx = event.position.x - innerBoxOrigin.x;
    const float my = event.position.y - innerBoxOrigin.y;

    if (selectedPoint == Attack)
    {
        const auto attackPosition = juce::jlimit (0.0f, width, mx);
        attackTime = juce::jmax<float> (0.01, attackPosition / width * durationWidth);
        parameters.getParameter (attackId)->setValueNotifyingHost (parameters.getParameterRange (attackId).convertTo0to1 (attackTime));
    }

    else if (selectedPoint == AttackCurve)
    {
        float graphValue = juce::jlimit (0.001f, 0.999f, 1.0f - my / height);
        float k = CurveUtils::inverseCurveAtHalf (graphValue);
        attackCurve = juce::jlimit (-1.0f, 1.0f, k);
        float normalized = parameters.getParameterRange (attackCurveId).convertTo0to1 (attackCurve);
        parameters.getParameter (attackCurveId)->setValueNotifyingHost (normalized);
    }

    else if (selectedPoint == Decay)
    {
        const auto decayPosition = juce::jlimit (attackX, width, mx);
        decayTime = juce::jmax<float> (0.01, (decayPosition - attackX) / width * durationWidth);
        parameters.getParameter (decayId)->setValueNotifyingHost (parameters.getParameterRange (decayId).convertTo0to1 (decayTime));

        sustainLevel = juce::jlimit (0.0f, height, my);
        parameters.getParameter (sustainId)->setValueNotifyingHost (parameters.getParameterRange (sustainId).convertTo0to1 (1 - (sustainLevel / height)));
    }

    else if (selectedPoint == DecayCurve)
    {
        float sustain = 1.0f - sustainLevel / height;
        float graphValue = juce::jlimit (0.001f, 0.999f, 1.0f - my / height);
        float t = juce::jlimit (0.001f, 0.999f, (1.0f - graphValue) / (1.0f - sustain));
        float k = CurveUtils::inverseCurveAtHalf (t);
        decayCurve = juce::jlimit (-1.0f, 1.0f, k);
        float normalized = parameters.getParameterRange (decayCurveId).convertTo0to1 (decayCurve);
        parameters.getParameter (decayCurveId)->setValueNotifyingHost (normalized);
    }

    else if (selectedPoint == Release)
    {
        const auto releasePosition = juce::jlimit (decayX, width, mx);
        releaseTime = juce::jmax<float> (0.01, (releasePosition - decayX) / width * durationWidth);
        const auto x = parameters.getParameterRange (releaseId).convertTo0to1 (releaseTime);
        parameters.getParameter (releaseId)->setValueNotifyingHost (x);
    }

    else if (selectedPoint == ReleaseCurve)
    {
        float sustain = 1.0f - sustainLevel / height;
        float graphValue = juce::jlimit (0.001f, 0.999f, 1.0f - my / height);
        float t = juce::jlimit (0.001f, 0.999f, (sustain - graphValue) / sustain);
        float k = CurveUtils::inverseCurveAtHalf (t);
        releaseCurve = juce::jlimit (-1.0f, 1.0f, k);
        float normalized = parameters.getParameterRange (releaseCurveId).convertTo0to1 (releaseCurve);
        parameters.getParameter (releaseCurveId)->setValueNotifyingHost (normalized);
    }

    repaint();
}

void ADSRGraph::mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    xOffset = juce::jlimit (0.0f, width * 2, xOffset - wheel.deltaX * 100);

    repaint();
}

void ADSRGraph::mouseMagnify (const juce::MouseEvent& event, float scaleFactor)
{
    durationWidth *= 1 + (1 - scaleFactor);

    repaint();
}