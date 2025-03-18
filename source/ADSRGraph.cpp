#include "ADSRGraph.h"

#include <utility>

// clang-format off
ADSRGraph::ADSRGraph(juce::AudioProcessorValueTreeState& p,
    const juce::StringRef attackParam,
    const juce::StringRef decayParam,
    const juce::StringRef sustainParam,
    const juce::StringRef releaseParam,
    std::shared_ptr<MyADSR*> adsr)
    : parameters(p), attackId(attackParam), decayId(decayParam), sustainId(sustainParam), releaseId(releaseParam), myADSR(std::move(adsr))
// clang-format on
{
    parameters.addParameterListener (attackId, this);
    parameters.addParameterListener (decayId, this);
    parameters.addParameterListener (sustainId, this);
    parameters.addParameterListener (releaseId, this);

    selectedPoint = None;

    attackTime = parameters.getRawParameterValue (attackId)->load();
    decayTime = parameters.getRawParameterValue (decayId)->load();
    sustainLevel = parameters.getRawParameterValue (sustainId)->load();
    releaseTime = parameters.getRawParameterValue (releaseId)->load();

    totalDuration = attackTime + decayTime + releaseTime;

    setSize (400, 200);

    startTimerHz (60);
}

ADSRGraph::~ADSRGraph()
{
    // parameters.state.removeListener (this);
    parameters.removeParameterListener (attackId, this);
    parameters.removeParameterListener (decayId, this);
    parameters.removeParameterListener (sustainId, this);
    parameters.removeParameterListener (releaseId, this);
}

void ADSRGraph::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == attackId)
        attackTime = newValue;
    else if (parameterID == decayId)
        decayTime = newValue;
    else if (parameterID == sustainId)
        sustainLevel = (1 - newValue) * static_cast<float> (getHeight());
    else if (parameterID == releaseId)
        releaseTime = newValue;

    repaint();
}

void ADSRGraph::setParameters (const float attack, const float decay, const float sustain, const float release)
{
    attackTime = attack;
    decayTime = decay;
    sustainLevel = (1 - sustain) * static_cast<float> (getHeight());
    releaseTime = release;
    repaint();
}

void ADSRGraph::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (30, 35, 35));
    g.setColour (juce::Colour::fromRGB (200, 200, 200));

    const auto bounds = getLocalBounds().toFloat();
    const float width = bounds.getWidth();
    const float height = bounds.getHeight();

    attackPoint = { attackTime / durationWidth * width - xOffset, 0 };
    decayPoint = { (attackTime + decayTime) / durationWidth * width - xOffset, sustainLevel };
    releasePoint = { (attackTime + decayTime + releaseTime) / durationWidth * width - xOffset, height };

    juce::Path attackCircle;
    attackCircle.addEllipse (attackPoint.getX() - 10, attackPoint.getY() - 10, 20, 20);
    g.fillPath (attackCircle);

    juce::Path decayCircle;
    decayCircle.addEllipse (decayPoint.getX() - 10, decayPoint.getY() - 10, 20, 20);
    g.fillPath (decayCircle);

    juce::Path releaseCircle;
    releaseCircle.addEllipse (releasePoint.getX() - 10, releasePoint.getY() - 10, 20, 20);
    g.fillPath (releaseCircle);

    juce::Path timeCircle;
    timeCircle.addEllipse (timePoint.getX() - 5, timePoint.getY() - 5, 10, 10);
    g.fillPath (timeCircle);

    juce::Path adsrPath;
    // adsrPath.startNewSubPath (0 - xOffset, height);
    // adsrPath.lineTo (attackPoint);
    // adsrPath.lineTo (decayPoint);
    // adsrPath.lineTo (releasePoint);
    drawCurve (g, attackPoint.getX(), decayPoint.getX(), releasePoint.getX());

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

void ADSRGraph::drawCurve (juce::Graphics& g, const int attackX, const int decayX, const int releaseX) const
{
    juce::Path path;

    const auto width = getWidth();
    const auto height = getHeight();
    constexpr auto startX = 0.0f;
    constexpr auto step = 2.0f;

    bool firstPoint = true;
    for (float x = startX; x <= releaseX; x += step)
    {
        const float y = getCurveY (x, width, height, attackX, decayX, releaseX);

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

float ADSRGraph::getCurveY (const float x, const int width, const int height, const int attackX, const int decayX, const int releaseX) const
{
    if (x < attackX)
        return (1 - MyADSR::toAttackCurve (x / attackTime / width * durationWidth, *parameters.getRawParameterValue ("ampAttackCurve"))) * height;

    if (x < decayX)
        return (1 - MyADSR::toDecayCurve ((x - attackX) / juce::jmax<float> (0.0001, decayTime) / width * durationWidth, 1 - sustainLevel / height, *parameters.getRawParameterValue ("ampDecayCurve"))) * height;

    if (x < releasePoint.getX())
        return (1 - MyADSR::toReleaseCurve ((x - decayX) / juce::jmax<float> (0.0001, releaseTime) / width * durationWidth, 1 - sustainLevel / height, *parameters.getRawParameterValue ("ampReleaseCurve"))) * height;

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

void ADSRGraph::mouseDown (const juce::MouseEvent& event)
{
    clickPoint = { event.position.x, event.position.y };
    constexpr float clickWidth = 20;

    if (clickPoint.getDistanceFrom (attackPoint) < clickWidth)
        selectedPoint = Attack;
    else if (clickPoint.getDistanceFrom (decayPoint) < clickWidth)
        selectedPoint = Decay;
    else if (clickPoint.getDistanceFrom (releasePoint) < clickWidth)
        selectedPoint = Release;
    else
        selectedPoint = None;
}

void ADSRGraph::mouseUp (const juce::MouseEvent& event)
{
    selectedPoint = None;
}

void ADSRGraph::mouseDrag (const juce::MouseEvent& event)
{
    const auto width = static_cast<float> (getWidth());
    const auto height = static_cast<float> (getHeight());

    if (selectedPoint == Attack)
    {
        const auto attackPosition = juce::jlimit (0.0f, width, event.position.x);
        attackTime = attackPosition / width * durationWidth;
        parameters.getParameter (attackId)->setValueNotifyingHost (parameters.getParameterRange (attackId).convertTo0to1 (attackTime));
    }

    else if (selectedPoint == Decay)
    {
        const auto decayPosition = juce::jlimit (attackPoint.getX(), width, event.position.x);
        decayTime = (decayPosition - attackPoint.getX()) / width * durationWidth;
        parameters.getParameter (decayId)->setValueNotifyingHost (parameters.getParameterRange (decayId).convertTo0to1 (decayTime));

        sustainLevel = juce::jlimit (0.0f, height, event.position.y);
        parameters.getParameter (sustainId)->setValueNotifyingHost (parameters.getParameterRange (sustainId).convertTo0to1 (1 - (sustainLevel / height)));
    }

    else if (selectedPoint == Release)
    {
        const auto releasePosition = juce::jlimit (decayPoint.getX(), width, event.position.x);
        releaseTime = (releasePosition - decayPoint.getX()) / width * durationWidth;
        const auto x = parameters.getParameterRange (releaseId).convertTo0to1 (releaseTime);
        parameters.getParameter (releaseId)->setValueNotifyingHost (x);
    }

    repaint();
}

void ADSRGraph::mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    // durationWidth *= 1 + wheel.deltaY;

    xOffset = juce::jlimit (0.0f, static_cast<float> (getWidth()) * 2, xOffset - wheel.deltaX * 100);

    repaint();
}

void ADSRGraph::mouseMagnify (const juce::MouseEvent& event, float scaleFactor)
{
    durationWidth *= 1 + (1 - scaleFactor);

    repaint();
}