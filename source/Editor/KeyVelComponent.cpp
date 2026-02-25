//
// KeyVelComponent.cpp
//
#include "KeyVelComponent.h"

KeyVelComponent::KeyVelComponent (juce::AudioProcessorValueTreeState& apvts,
                                  juce::UndoManager* um,
                                  std::atomic<int>* gestureCount,
                                  ModulationModeState* modState)
    : parameters (apvts),
      undoManager (um),
      activeGestureCount (gestureCount),
      modModeState (modState)
{
    velCurveParam = parameters.getParameter ("velocityCurve");
    keyCurveParam = parameters.getParameter ("keyboardCurve");

    if (modModeState != nullptr)
        modModeState->addListener (this);
}

KeyVelComponent::~KeyVelComponent()
{
    if (modModeState != nullptr)
        modModeState->removeListener (this);
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

void KeyVelComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Tab strip
    auto tabArea = bounds.removeFromTop (kTabHeight);
    int tabWidth = tabArea.getWidth() / 2;

    const juce::String sourceIDs[] = { "velocity", "keyboard" };
    const juce::String tabLabels[] = { "Vel", "Key" };

    for (int i = 0; i < 2; ++i)
    {
        auto tab = tabArea.removeFromLeft (tabWidth);
        g.setColour (i == activeTab ? juce::Colours::orange : SECONDARY_COLOR);
        g.fillRect (tab);

        // Crosshair icon
        auto tabF = tab.toFloat();
        const float iconSize = 16.0f;
        auto iconArea = tabF.withWidth (iconSize + 8).reduced (4.0f);
        float cx = iconArea.getCentreX();
        float cy = iconArea.getCentreY();
        float r = iconSize * 0.35f;

        bool isTarget = modModeState != nullptr
            && modModeState->isModulationMode()
            && modModeState->getTargetSourceID() == sourceIDs[i];

        g.setColour (isTarget ? MOD_COLOR : TEXT_COLOR.withAlpha (0.6f));
        g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 1.5f);
        g.drawLine (cx - r - 2, cy, cx + r + 2, cy, 1.2f);
        g.drawLine (cx, cy - r - 2, cx, cy + r + 2, 1.2f);

        // Text label
        g.setColour (i == activeTab ? juce::Colours::black : TEXT_COLOR);
        g.setFont (13.0f);
        g.drawText (tabLabels[i], tabF.withTrimmedLeft (iconSize + 8), juce::Justification::centred, true);
    }

    // Graph area — force square aspect ratio
    auto graphRect = bounds.toFloat().reduced (4.0f);
    float side = juce::jmin (graphRect.getWidth(), graphRect.getHeight());
    auto graphArea = graphRect.withSizeKeepingCentre (side, side);
    g.setColour (SECONDARY_COLOR);
    g.fillRoundedRectangle (graphArea, 3.0f);

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
    g.setColour (juce::Colours::orange);
    g.strokePath (path, juce::PathStrokeType (2.0f));

    // Draw handle at x=0.5 midpoint
    float handleX = graphArea.getX() + 0.5f * graphArea.getWidth();
    float handleY = graphArea.getBottom() - VelocitySource::applyCurve (0.5f, curve) * graphArea.getHeight();
    g.setColour (juce::Colours::white);
    g.fillEllipse (handleX - 5.0f, handleY - 5.0f, 10.0f, 10.0f);
    g.setColour (juce::Colours::orange);
    g.drawEllipse (handleX - 5.0f, handleY - 5.0f, 10.0f, 10.0f, 1.5f);
}

void KeyVelComponent::resized()
{
}

void KeyVelComponent::mouseDown (const juce::MouseEvent& e)
{
    auto tabArea = getLocalBounds().removeFromTop (kTabHeight);
    if (e.y < kTabHeight)
    {
        int newTab = e.x < tabArea.getWidth() / 2 ? 0 : 1;
        int tabLeft = newTab * (tabArea.getWidth() / 2);
        bool clickedCrosshair = (e.x - tabLeft) < 24;

        const juce::String sourceIDs[] = { "velocity", "keyboard" };

        if (modModeState != nullptr)
        {
            if (clickedCrosshair && !modModeState->isModulationMode())
            {
                modModeState->setTargetSource (sourceIDs[newTab]);
                modModeState->setMode (ModulationModeState::Mode::Modulation);
                activeTab = newTab;
                repaint();
                return;
            }
            if (modModeState->isModulationMode())
            {
                modModeState->setTargetSource (sourceIDs[newTab]);
                activeTab = newTab;
                repaint();
                return;
            }
        }

        if (newTab != activeTab)
        {
            activeTab = newTab;
            repaint();
        }
        return;
    }

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

void KeyVelComponent::updateCurveFromDrag (const juce::MouseEvent& e)
{
    auto graphRect = getLocalBounds().withTrimmedTop (kTabHeight).toFloat().reduced (4.0f);
    float side = juce::jmin (graphRect.getWidth(), graphRect.getHeight());
    auto graphArea = graphRect.withSizeKeepingCentre (side, side);

    // Map vertical position to curve value [-1, 1]
    // Top = -1 (concave), bottom = +1 (convex), center = 0 (linear)
    float normalizedY = 1.0f - (static_cast<float> (e.y) - graphArea.getY()) / graphArea.getHeight();
    // Map so dragging handle up (higher output at 0.5) = positive curve (convex)
    // and dragging down = negative curve (concave)
    float curveValue = juce::jlimit (-1.0f, 1.0f, (normalizedY - 0.5f) * 2.0f);

    if (auto* p = getCurrentParam())
    {
        float normalized = p->getNormalisableRange().convertTo0to1 (curveValue);
        p->setValueNotifyingHost (normalized);
        repaint();
    }
}
