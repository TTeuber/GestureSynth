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
    std::atomic<int>* gestureCount)
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
    gestureCount(gestureCount)
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
    sustainLevel = parameters.getRawParameterValue (sustainId)->load();
    releaseTime = parameters.getRawParameterValue (releaseId)->load();
    releaseCurve = parameters.getRawParameterValue (releaseCurveId)->load();

    totalDuration = attackTime + decayTime + releaseTime;

    startTimerHz (60);
}

ADSRGraph::~ADSRGraph()
{
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
    sustainLevel = parameters.getRawParameterValue (sustainId)->load();
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
        sustainLevel = (1 - newValue) * static_cast<float> (getHeight());
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
    sustainLevel = (1 - sustain) * static_cast<float> (getHeight());
    releaseTime = release;
    repaint();
}

void ADSRGraph::resized()
{
    width = static_cast<float> (getWidth());
    height = static_cast<float> (getHeight());
    sustainLevel = (1 - parameters.getRawParameterValue (sustainId)->load()) * height;
}

void ADSRGraph::paint (juce::Graphics& g)
{
    g.fillAll (SECONDARY_COLOR);
    g.setColour (TEXT_COLOR);

    attackX = attackTime / durationWidth * width - xOffset;
    attackPoint = { attackX, 0 };

    decayX = (attackTime + decayTime) / durationWidth * width - xOffset;
    decayPoint = { decayX, sustainLevel };

    releaseX = (attackTime + decayTime + releaseTime) / durationWidth * width - xOffset;
    releasePoint = { releaseX, (height) };

    attackCurvePoint = { attackX / 2, getCurveY (attackX / 2) };

    float decayCurveX = (decayX - attackX) / 2 + attackX;
    decayCurvePoint = { decayCurveX, getCurveY (decayCurveX) };

    float releaseCurveX = (releaseX - decayX) / 2 + decayX;
    releaseCurvePoint = { releaseCurveX, getCurveY (releaseCurveX) };

    juce::Path attackCircle;
    attackCircle.addEllipse (attackX - 10, attackPoint.getY() - 10, 20, 20);
    g.fillPath (attackCircle);

    juce::Path attackCurveCircle;
    attackCurveCircle.addEllipse (attackX / 2 - 5, getCurveY (attackX / 2) - 5, 10, 10);
    g.fillPath (attackCurveCircle);

    juce::Path decayCircle;
    decayCircle.addEllipse (decayX - 10, decayPoint.getY() - 10, 20, 20);
    g.fillPath (decayCircle);

    juce::Path decayCurveCircle;
    decayCurveCircle.addEllipse (decayCurveX - 5, getCurveY (decayCurveX) - 5, 10, 10);
    g.fillPath (decayCurveCircle);

    juce::Path releaseCircle;
    releaseCircle.addEllipse (releaseX - 10, releasePoint.getY() - 10, 20, 20);
    g.fillPath (releaseCircle);

    juce::Path releaseCurveCircle;
    releaseCurveCircle.addEllipse (releaseCurveX - 5, getCurveY (releaseCurveX) - 5, 10, 10);
    g.fillPath (releaseCurveCircle);

    juce::Path timeCircle;
    timeCircle.addEllipse (timePoint.getX() - 5, timePoint.getY() - 5, 10, 10);
    g.fillPath (timeCircle);

    juce::Path adsrPath;

    drawCurve (g);

    for (float i = 0; i < durationWidth * 3; i += 1.0f)
    {
        const auto markerPosition = width / durationWidth * (i + 1);

        juce::Path marker;
        marker.startNewSubPath (markerPosition - xOffset, 0);
        marker.lineTo (markerPosition - xOffset, height);
        g.strokePath (marker, juce::PathStrokeType (2.0f));
        g.drawText (juce::String (i + 1), markerPosition - (durationWidth / 10) - xOffset, height - 20, 20, 20, juce::Justification::centred);

        if (durationWidth < 6.0f)
        {
            juce::Path subMarker;
            subMarker.startNewSubPath (markerPosition - xOffset - width / durationWidth / 2, 0);
            subMarker.lineTo (markerPosition - xOffset - width / durationWidth / 2, height);
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

    bool firstPoint = true;
    for (float x = startX; x <= releaseX; x += step)
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
    if (myADSR != nullptr && *myADSR != nullptr)
    {
        const std::array<float, 2> xy = (*myADSR)->getTimePoint();
        timePoint = { xy[0] / durationWidth * static_cast<float> (getWidth()) - xOffset, ((1 - xy[1]) * static_cast<float> (getHeight())) };
        repaint();
    }
}

void ADSRGraph::timerCallback()
{
    showTime();
}

void ADSRGraph::mouseMove (const juce::MouseEvent& event)
{
    for (auto p : points)
    {
        if (p->getDistanceFrom (juce::Point (event.position.x, event.position.y)) < 20)
        {
            setMouseCursor (juce::MouseCursor::PointingHandCursor);
            return;
        }
    }
    setMouseCursor (juce::MouseCursor::NormalCursor);
}

void ADSRGraph::mouseDown (const juce::MouseEvent& event)
{
    clickPoint = { event.position.x, event.position.y };
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
    const auto width = static_cast<float> (getWidth());
    const auto height = static_cast<float> (getHeight());

    if (selectedPoint == Attack)
    {
        const auto attackPosition = juce::jlimit (0.0f, width, event.position.x);
        attackTime = juce::jmax<float> (0.01, attackPosition / width * durationWidth);
        parameters.getParameter (attackId)->setValueNotifyingHost (parameters.getParameterRange (attackId).convertTo0to1 (attackTime));
    }

    else if (selectedPoint == AttackCurve)
    {
        attackCurve = juce::jlimit (0.0f, height, event.position.y) / height;
        parameters.getParameter (attackCurveId)->setValueNotifyingHost (attackCurve);
    }

    else if (selectedPoint == Decay)
    {
        const auto decayPosition = juce::jlimit (attackX, width, event.position.x);
        decayTime = juce::jmax<float> (0.01, (decayPosition - attackX) / width * durationWidth);
        parameters.getParameter (decayId)->setValueNotifyingHost (parameters.getParameterRange (decayId).convertTo0to1 (decayTime));

        sustainLevel = juce::jlimit (0.0f, height, event.position.y);
        parameters.getParameter (sustainId)->setValueNotifyingHost (parameters.getParameterRange (sustainId).convertTo0to1 (1 - (sustainLevel / height)));
    }

    else if (selectedPoint == DecayCurve)
    {
        decayCurve = 1 - juce::jlimit (0.0f, sustainLevel, event.position.y) / sustainLevel;
        parameters.getParameter (decayCurveId)->setValueNotifyingHost (decayCurve);
    }

    else if (selectedPoint == Release)
    {
        const auto releasePosition = juce::jlimit (decayX, width, event.position.x);
        releaseTime = juce::jmax<float> (0.01, (releasePosition - decayX) / width * durationWidth);
        const auto x = parameters.getParameterRange (releaseId).convertTo0to1 (releaseTime);
        parameters.getParameter (releaseId)->setValueNotifyingHost (x);
    }

    else if (selectedPoint == ReleaseCurve)
    {
        releaseCurve = 1 - juce::jlimit (0.0f, height - sustainLevel, event.position.y - sustainLevel) / (height - sustainLevel);
        parameters.getParameter (releaseCurveId)->setValueNotifyingHost (releaseCurve);
    }

    repaint();
}

void ADSRGraph::mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    xOffset = juce::jlimit (0.0f, static_cast<float> (getWidth()) * 2, xOffset - wheel.deltaX * 100);

    repaint();
}

void ADSRGraph::mouseMagnify (const juce::MouseEvent& event, float scaleFactor)
{
    durationWidth *= 1 + (1 - scaleFactor);

    repaint();
}