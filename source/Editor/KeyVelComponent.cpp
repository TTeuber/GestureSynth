//
// KeyVelComponent.cpp
//
#include "KeyVelComponent.h"
#include "../Utility/AtomicHelpers.h"
#include "../Utility/Parameters.h"

KeyVelComponent::KeyVelComponent (juce::AudioProcessorValueTreeState& apvts,
                                  juce::UndoManager* um,
                                  std::atomic<int>* gestureCount,
                                  ModulationModeState* modState,
                                  std::atomic<float>* velocityRaw,
                                  std::atomic<float>* keyboardRaw,
                                  AnimationFrameSource* animSource,
                                  std::shared_ptr<MyADSR*> ampEnv)
    : parameters (apvts),
      undoManager (um),
      activeGestureCount (gestureCount),
      modModeState (modState),
      velocityRawPtr (velocityRaw),
      keyboardRawPtr (keyboardRaw),
      animSource (animSource),
      ampEnvPtr (std::move (ampEnv))
{
    velCurveParam = parameters.getParameter (ParamIDs::velocityCurve);
    keyCurveParam = parameters.getParameter (ParamIDs::keyboardCurve);

    if (modModeState != nullptr)
        modModeState->addListener (this);

    if (animSource != nullptr)
        animSource->addListener (this, AnimationFrameSource::Rate::Hz60);
}

KeyVelComponent::~KeyVelComponent()
{
    if (animSource != nullptr)
        animSource->removeListener (this);
    if (modModeState != nullptr)
        modModeState->removeListener (this);
}

void KeyVelComponent::onAnimationFrame()
{
    bool noteActive = ampEnvPtr != nullptr && *ampEnvPtr != nullptr && (*ampEnvPtr)->isActive();
    bool needsRepaint = false;

    auto* ptr = (activeTab == 0) ? velocityRawPtr : keyboardRawPtr;
    if (ptr != nullptr)
    {
        float newVal = AtomicHelpers::paramLoad (*ptr);
        if (newVal != currentInputValue)
        {
            currentInputValue = newVal;
            needsRepaint = true;
        }
    }

    if (noteActive != wasNoteActive)
    {
        wasNoteActive = noteActive;
        needsRepaint = true;
    }

    if (needsRepaint)
        repaint();
}

void KeyVelComponent::modulationModeChanged (ModulationModeState::Mode)
{
    repaint();
}

void KeyVelComponent::targetSourceChanged (const juce::String&)
{
    repaint();
}

juce::RangedAudioParameter* KeyVelComponent::getCurrentParam() const
{
    return activeTab == 0 ? velCurveParam : keyCurveParam;
}

float KeyVelComponent::getCurveParam() const
{
    auto* p = getCurrentParam();
    return p->getNormalisableRange().convertFrom0to1 (p->getValue());
}

juce::Rectangle<float> KeyVelComponent::getGraphArea() const
{
    auto graphRect = getLocalBounds().toFloat().reduced (4.0f);
    float side = juce::jmin (graphRect.getWidth(), graphRect.getHeight());
    return graphRect.withSizeKeepingCentre (side, side);
}

bool KeyVelComponent::isOverHandle (const juce::MouseEvent& e) const
{
    auto graphArea = getGraphArea();
    float curve = getCurveParam();
    float handleX = graphArea.getX() + 0.5f * graphArea.getWidth();
    float handleY = graphArea.getBottom() - VelocitySource::applyCurve (0.5f, curve) * graphArea.getHeight();
    float dx = static_cast<float> (e.x) - handleX;
    float dy = static_cast<float> (e.y) - handleY;
    return std::sqrt (dx * dx + dy * dy) <= 12.0f;
}

void KeyVelComponent::paint (juce::Graphics& g)
{
    auto graphArea = getGraphArea();
    g.setColour (SECONDARY_COLOR);
    g.fillRoundedRectangle (graphArea, Style::radiusSmall);

    float curve = getCurveParam();

    // Draw curve path
    juce::Path path;
    const int numPoints = 64;
    for (int i = 0; i <= numPoints; ++i)
    {
        float x = static_cast<float> (i) / static_cast<float> (numPoints);
        float y = VelocitySource::applyCurve (x, curve);
        float px = graphArea.getX() + x * graphArea.getWidth();
        float py = graphArea.getBottom() - y * graphArea.getHeight();
        if (i == 0)
            path.startNewSubPath (px, py);
        else
            path.lineTo (px, py);
    }
    g.setColour (juce::Colours::white);
    g.strokePath (path, juce::PathStrokeType (2.0f));

    // Draw handle at x=0.5 midpoint
    float handleX = graphArea.getX() + 0.5f * graphArea.getWidth();
    float handleY = graphArea.getBottom() - VelocitySource::applyCurve (0.5f, curve) * graphArea.getHeight();

    // Hover ring (matches ADSR/LFO hover effect)
    if (handleHovered || dragging)
    {
        g.setColour (juce::Colours::white.withAlpha (0.3f));
        g.fillEllipse (handleX - 10.0f, handleY - 10.0f, 20.0f, 20.0f);
    }

    g.setColour (juce::Colours::white);
    g.fillEllipse (handleX - 5.0f, handleY - 5.0f, 10.0f, 10.0f);

    // Draw current value indicator only when a note is playing
    bool noteActive = ampEnvPtr != nullptr && *ampEnvPtr != nullptr && (*ampEnvPtr)->isActive();
    if (noteActive)
    {
        float x = currentInputValue;
        float y = VelocitySource::applyCurve (x, curve);
        float px = graphArea.getX() + x * graphArea.getWidth();
        float py = graphArea.getBottom() - y * graphArea.getHeight();
        g.setColour (juce::Colours::white);
        g.fillEllipse (px - 4.0f, py - 4.0f, 8.0f, 8.0f);
    }
}

void KeyVelComponent::resized()
{
}

void KeyVelComponent::mouseDown (const juce::MouseEvent& e)
{
    dragging = true;
    if (auto* p = getCurrentParam())
    {
        p->beginChangeGesture();
        if (activeGestureCount != nullptr)
            ++(*activeGestureCount);
    }
    updateCurveFromDrag (e);
}

void KeyVelComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging)
        updateCurveFromDrag (e);
}

void KeyVelComponent::mouseUp (const juce::MouseEvent&)
{
    if (dragging)
    {
        dragging = false;
        if (auto* p = getCurrentParam())
        {
            p->endChangeGesture();
            if (activeGestureCount != nullptr)
                --(*activeGestureCount);
        }
    }
}

void KeyVelComponent::mouseMove (const juce::MouseEvent& e)
{
    bool over = isOverHandle (e);
    if (over != handleHovered)
    {
        handleHovered = over;
        setMouseCursor (over ? juce::MouseCursor::PointingHandCursor
                             : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void KeyVelComponent::mouseExit (const juce::MouseEvent&)
{
    if (handleHovered)
    {
        handleHovered = false;
        setMouseCursor (juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void KeyVelComponent::updateCurveFromDrag (const juce::MouseEvent& e)
{
    auto graphArea = getGraphArea();

    // Normalized Y in [0, 1]: 0 = bottom of graph, 1 = top
    float normalizedY = 1.0f - (static_cast<float> (e.y) - graphArea.getY()) / graphArea.getHeight();
    float safeY = juce::jlimit (0.001f, 0.999f, normalizedY);

    float curveValue = juce::jlimit (-1.0f, 1.0f, CurveUtils::inverseCurveAtHalf (safeY));

    if (auto* p = getCurrentParam())
    {
        float normalized = p->getNormalisableRange().convertTo0to1 (curveValue);
        p->setValueNotifyingHost (normalized);
        repaint();
    }
}
