#include "ADSRGraph.h"

// clang-format off
ADSRGraph::ADSRGraph(juce::AudioProcessorValueTreeState& p,
    juce::StringRef attackParam,
    juce::StringRef decayParam,
    juce::StringRef sustainParam,
    juce::StringRef releaseParam)
    : parameters(p), attackId(attackParam), decayId(decayParam), sustainId(sustainParam), releaseId(releaseParam)
// clang-format on
{
    parameters.state.addListener (this);

    selectedPoint = None;

    attackTime = parameters.getRawParameterValue (attackId)->load();
    decayTime = parameters.getRawParameterValue (decayId)->load();
    sustainLevel = parameters.getRawParameterValue (sustainId)->load();
    releaseTime = parameters.getRawParameterValue (releaseId)->load();

    totalDuration = attackTime + decayTime + releaseTime;

    setSize (400, 200);
}

ADSRGraph::~ADSRGraph()
{
    parameters.state.removeListener (this);
}

void ADSRGraph::valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property)
{
    // Update the ADSRGraph based on the changed parameters
    auto attack = parameters.getRawParameterValue (attackId)->load();
    auto decay = parameters.getRawParameterValue (decayId)->load();
    auto sustain = parameters.getRawParameterValue (sustainId)->load();
    auto release = parameters.getRawParameterValue (releaseId)->load();

    // Ensure the graph reflects these new values
    setParameters (attack, decay, sustain, release);
    repaint(); // Redraw the ADSR graph
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
    g.fillAll (juce::Colours::grey);
    g.setColour (juce::Colours::white);

    auto bounds = getLocalBounds().toFloat();
    float width = bounds.getWidth();
    float height = bounds.getHeight();

    juce::Path adsrPath;
    attackPoint = { attackTime / durationWidth * width, 0 };
    decayPoint = { (attackTime + decayTime) / durationWidth * width, sustainLevel };
    releasePoint = { (attackTime + decayTime + releaseTime) / durationWidth * width, height };

    juce::Path attackCircle;
    attackCircle.addEllipse (attackPoint.getX() - 10, attackPoint.getY() - 10, 20, 20);
    g.fillPath (attackCircle);

    juce::Path decayCircle;
    decayCircle.addEllipse (decayPoint.getX() - 10, decayPoint.getY() - 10, 20, 20);
    g.fillPath (decayCircle);

    juce::Path releaseCircle;
    releaseCircle.addEllipse (releasePoint.getX() - 10, releasePoint.getY() - 10, 20, 20);
    g.fillPath (releaseCircle);

    adsrPath.startNewSubPath (0, height);
    adsrPath.lineTo (attackPoint);
    adsrPath.lineTo (decayPoint);
    adsrPath.lineTo (releasePoint);

    for (float i = 0; i < durationWidth; i += 1.0f)
    {
        const auto x = width / durationWidth * (i + 1);

        juce::Path marker;
        marker.startNewSubPath (x, 0);
        marker.lineTo (x, height);
        g.strokePath (marker, juce::PathStrokeType (1.0f));
    }

    g.strokePath (adsrPath, juce::PathStrokeType (4.0f));
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
        decayTime = ((decayPosition - attackPoint.getX()) / width) * durationWidth;
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
    durationWidth *= 1 + wheel.deltaY;

    repaint();
}

void ADSRGraph::mouseMagnify (const juce::MouseEvent& event, float scaleFactor)
{
    durationWidth *= 1 + (1 - scaleFactor);

    repaint();
}