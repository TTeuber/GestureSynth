//
// High Pass Filter Display Component
//

#include "HPFDisplay.h"
#include "../Utility/Parameters.h"

HPFDisplay::HPFDisplay (juce::AudioProcessorValueTreeState& apvtsToUse,
    const UIContext& ctx)
    : apvts (apvtsToUse), undoManager (ctx.undoManager), gestureCount (ctx.gestureCount),
      modModeState (ctx.modModeState)
{
    this->apvts.addParameterListener (ParamIDs::hpfFrequency, this);
    this->apvts.addParameterListener (ParamIDs::hpfOn, this);

    cutoffParam = apvts.getParameter (ParamIDs::hpfFrequency);

    auto* hpfOnParam = apvts.getParameter (ParamIDs::hpfOn);
    if (hpfOnParam != nullptr)
        hpfEnabled = hpfOnParam->getValue() > 0.5f;

    if (modModeState != nullptr)
        modModeState->addListener (this);

    updateParameterValues();

    setSize (200, 150);
}

HPFDisplay::~HPFDisplay()
{
    if (modModeState != nullptr)
        modModeState->removeListener (this);
    apvts.removeParameterListener (ParamIDs::hpfFrequency, this);
    apvts.removeParameterListener (ParamIDs::hpfOn, this);
}

void HPFDisplay::paint (juce::Graphics& g)
{
    const int labelH = static_cast<int> (Style::labelHeight);

    // Fill with parent background
    g.fillAll (PRIMARY_COLOR);

    // Outer box
    auto outerBounds = getLocalBounds();
    PaintHelpers::drawComponentBox (g, outerBounds.toFloat());

    // Layout: top label, inner box, bottom label
    auto innerArea = outerBounds.reduced (4);
    auto topLabelArea = innerArea.removeFromTop (labelH);
    innerArea.removeFromBottom (labelH);
    auto innerBoxBounds = innerArea.reduced (4);

    // Inner box
    PaintHelpers::drawInnerBox (g, innerBoxBounds.toFloat());

    // Draw label with fading opacity
    if (hoverAnimator.getAlpha() < 0.99f)
    {
        g.setColour (hpfEnabled ? TEXT_COLOR : TEXT_INACTIVE_COLOR);
        g.setFont (Style::fontComponent);
        g.setOpacity (1.0f - hoverAnimator.getAlpha());
        g.drawText ("High Pass Filter", topLabelArea, juce::Justification::centred, true);
    }

    // Draw parameter values with fading opacity
    if (hoverAnimator.getAlpha() > 0.01f)
        drawParameterValues (g);

    g.setOpacity (1.0f);

    // Clip visualization to inner box
    g.saveState();
    g.reduceClipRegion (innerBoxBounds);

    drawFrequencyPath (g);
    drawModModeOverlay (g);

    g.restoreState();
}

void HPFDisplay::resized()
{
}

void HPFDisplay::mouseDown (const juce::MouseEvent& e)
{
    if (inlineEditor.isEditing())
        return;

    if (e.mods.isPopupMenu())
        return;

    if (e.getNumberOfClicks() > 1)
        return;

    // Modulation mode handling
    if (modModeState != nullptr && modModeState->isModulationMode())
    {
        auto sourceID = modModeState->getTargetSourceID();

        int slot = modModeState->getOrCreateSlot (sourceID, ParamIDs::hpfFrequency);
        if (slot < 0)
            return;

        if (undoManager != nullptr)
            undoManager->beginNewTransaction();
        if (gestureCount != nullptr)
            ++(*gestureCount);

        modDragInitialDepth = modModeState->getDepth (sourceID, ParamIDs::hpfFrequency);
        modDragStartX = e.x;
        isModDragging = true;
        return;
    }

    if (hpfEnabled)
    {
        isDragging = true;
        dragStartX = e.x;
        dragStartCutoff = normalizedCutoff;

        if (undoManager != nullptr)
            undoManager->beginNewTransaction();
        if (cutoffParam)
            cutoffParam->beginChangeGesture();
        if (gestureCount != nullptr)
            ++(*gestureCount);
    }
}

void HPFDisplay::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (!hpfEnabled)
        return;

    if (modModeState != nullptr && modModeState->isModulationMode())
        return;

    if (getValueLabelBounds().contains (e.getPosition()))
        beginCutoffEdit();
}

void HPFDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (inlineEditor.isEditing())
        return;

    if (isModDragging && modModeState != nullptr)
    {
        auto sourceID = modModeState->getTargetSourceID();

        float hDelta = static_cast<float> (e.x - modDragStartX) / static_cast<float> (getWidth());
        float newDepth = juce::jlimit (-1.0f, 1.0f, modDragInitialDepth + hDelta);
        modModeState->setDepth (sourceID, ParamIDs::hpfFrequency, newDepth);
        repaint();
        return;
    }

    if (isDragging)
    {
        float horizontalDelta = (e.position.x - static_cast<float> (dragStartX)) / static_cast<float> (getWidth());
        float newCutoff = juce::jlimit (0.0f, 1.0f, dragStartCutoff + horizontalDelta);

        if (cutoffParam)
            cutoffParam->setValueNotifyingHost (newCutoff);

        repaint();
    }
}

void HPFDisplay::mouseUp (const juce::MouseEvent& e)
{
    if (inlineEditor.isEditing())
        return;

    if (e.mods.isPopupMenu() && e.mouseWasClicked() && getLocalBounds().contains (e.getPosition()))
    {
        if (modModeState != nullptr && modModeState->isModulationMode() && hpfEnabled)
        {
            auto sourceID = modModeState->getTargetSourceID();
            if (modModeState->findSlotIndex (sourceID, ParamIDs::hpfFrequency) >= 0)
            {
                showModulationContextMenu (this, modModeState,
                    { { cutoffParam->getName (15), sourceID, ParamIDs::hpfFrequency } },
                    e.getScreenPosition());
                return;
            }
        }

        auto* hpfOnParam = apvts.getParameter (ParamIDs::hpfOn);
        if (hpfOnParam != nullptr)
        {
            if (undoManager != nullptr)
                undoManager->beginNewTransaction();
            hpfOnParam->beginChangeGesture();
            hpfOnParam->setValueNotifyingHost (!hpfEnabled);
            hpfOnParam->endChangeGesture();
            repaint();
            return;
        }
    }

    if (isModDragging)
    {
        if (gestureCount != nullptr)
            --(*gestureCount);
        isModDragging = false;
        return;
    }
    if (isDragging)
    {
        if (cutoffParam)
            cutoffParam->endChangeGesture();
        if (gestureCount != nullptr)
            --(*gestureCount);
    }
    isDragging = false;
}

void HPFDisplay::mouseEnter (const juce::MouseEvent&)
{
    setMouseCursor (juce::MouseCursor::CrosshairCursor);
    hoverAnimator.setHovered (true);
}

void HPFDisplay::mouseExit (const juce::MouseEvent&)
{
    setMouseCursor (juce::MouseCursor::NormalCursor);
    hoverAnimator.setHovered (false);
}

void HPFDisplay::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == ParamIDs::hpfFrequency)
    {
        updateParameterValues();

        juce::MessageManager::callAsync ([this] {
            repaint();
        });
    }
    else if (parameterID == ParamIDs::hpfOn)
    {
        hpfEnabled = newValue > 0.5f;
        juce::MessageManager::callAsync ([this] {
            repaint();
        });
    }
}

void HPFDisplay::updateParameterValues()
{
    if (cutoffParam)
    {
        normalizedCutoff = cutoffParam->getValue();
        cutoffFrequency = cutoffParam->getNormalisableRange().convertFrom0to1 (normalizedCutoff);
    }

}

void HPFDisplay::drawFrequencyPath (juce::Graphics& g) const
{
    juce::Path path;
    constexpr int step = 5;

    // Start from below 20Hz (negative pixels) so the rolloff is always visible
    const int startPixel = static_cast<int> (freqToDisplayX (kCurveStartFreq) * getWidth());
    bool pathStarted = false;

    for (int xPixel = startPixel; xPixel < getWidth(); xPixel += step)
    {
        const double x = static_cast<double> (xPixel) / getWidth();
        const double freq = displayXToFreq (x);
        const double dB = getHPGainDb (freq);
        const float yPixel = static_cast<float> (getHeight()) / 2.0f - static_cast<float> (dB) * static_cast<float> (getHeight()) / 50.0f;

        if (! pathStarted)
        {
            path.startNewSubPath (static_cast<float> (xPixel), yPixel);
            pathStarted = true;
        }
        else
            path.lineTo (static_cast<float> (xPixel), yPixel);
    }

    g.setColour (hpfEnabled ? TEXT_COLOR : TEXT_INACTIVE_COLOR);
    g.strokePath (path, juce::PathStrokeType (2.0f));
}

double HPFDisplay::getHPGainDb (double freq) const
{
    return computeHPGainDb (freq, cutoffFrequency);
}

double HPFDisplay::computeHPGainDb (double freq, double cutoffFreqHz)
{
    if (cutoffFreqHz <= 0.0 || freq <= 0.0)
        return -60.0;

    const double w = freq / cutoffFreqHz;
    const double wSquared = w * w;
    const double q = 1.0 / std::sqrt (2.0); // ~0.707, Butterworth per stage
    const double denominator = std::sqrt ((1.0 - wSquared) * (1.0 - wSquared) + (w / q) * (w / q));

    if (denominator < 1e-10)
        return -60.0;

    // Two cascaded 2nd-order HP stages => square the magnitude (double the dB)
    const double gain = wSquared / denominator;

    if (gain < 1e-10)
        return -60.0;

    return 2.0 * 20.0 * std::log10 (gain);
}

void HPFDisplay::drawParameterValues (juce::Graphics& g) const
{
    g.setOpacity (hoverAnimator.getAlpha());
    g.setColour ((hpfEnabled ? TEXT_COLOR : TEXT_INACTIVE_COLOR).withAlpha (hoverAnimator.getAlpha()));
    g.setFont (Style::fontComponent);

    juce::String freqText;
    if (cutoffFrequency < 1000.0f)
        freqText = juce::String (static_cast<int> (cutoffFrequency)) + " Hz";
    else
        freqText = juce::String (cutoffFrequency / 1000.0f, 1) + " kHz";

    g.drawText ("HPF: " + freqText, getValueLabelBounds(), juce::Justification::centred, true);
}

juce::Rectangle<int> HPFDisplay::getValueLabelBounds() const
{
    auto innerArea = getLocalBounds().reduced (4);
    return innerArea.removeFromTop (static_cast<int> (Style::labelHeight));
}

juce::String HPFDisplay::getCutoffDisplayText() const
{
    juce::String freqText;
    if (cutoffFrequency < 1000.0f)
        freqText = juce::String (static_cast<int> (cutoffFrequency)) + " Hz";
    else
        freqText = juce::String (cutoffFrequency / 1000.0f, 1) + " kHz";

    return "HPF: " + freqText;
}

juce::String HPFDisplay::getCutoffEditText() const
{
    return InlineParameterEditUtils::extractEditableText (getCutoffDisplayText());
}

void HPFDisplay::beginCutoffEdit()
{
    if (cutoffParam == nullptr)
        return;

    inlineEditor.beginEdit (getValueLabelBounds().reduced (2), getCutoffEditText(),
        [this] (const juce::String& text)
        {
            commitCutoffText (text);
        });
}

void HPFDisplay::commitCutoffText (const juce::String& text)
{
    if (cutoffParam == nullptr)
        return;

    if (undoManager != nullptr)
        undoManager->beginNewTransaction();

    cutoffParam->beginChangeGesture();
    if (gestureCount != nullptr)
        ++(*gestureCount);
    cutoffParam->setValueNotifyingHost (InlineParameterEditUtils::parseNormalizedValue (cutoffParam, text, getCutoffDisplayText()));
    cutoffParam->endChangeGesture();
    if (gestureCount != nullptr)
        --(*gestureCount);
}

void HPFDisplay::drawHPFCurveAt (juce::Graphics& g, float cutoffFreqHz,
    juce::Colour colour, float strokeWidth) const
{
    if (cutoffFreqHz <= 0.0f)
        return;

    juce::Path path;
    constexpr int step = 5;

    const int startPixel = static_cast<int> (freqToDisplayX (kCurveStartFreq) * getWidth());
    bool pathStarted = false;

    for (int xPixel = startPixel; xPixel < getWidth(); xPixel += step)
    {
        const double x = static_cast<double> (xPixel) / getWidth();
        const double freq = displayXToFreq (x);
        const double dB = computeHPGainDb (freq, cutoffFreqHz);
        const float yPixel = static_cast<float> (getHeight()) / 2.0f - static_cast<float> (dB) * static_cast<float> (getHeight()) / 50.0f;

        if (! pathStarted)
        {
            path.startNewSubPath (static_cast<float> (xPixel), yPixel);
            pathStarted = true;
        }
        else
            path.lineTo (static_cast<float> (xPixel), yPixel);
    }

    g.setColour (colour);
    g.strokePath (path, juce::PathStrokeType (strokeWidth));
}

void HPFDisplay::drawModModeOverlay (juce::Graphics& g) const
{
    if (modModeState == nullptr || !modModeState->isModulationMode())
        return;

    auto sourceID = modModeState->getTargetSourceID();
    float depth = modModeState->getDepth (sourceID, ParamIDs::hpfFrequency);


    bool bipolar = modModeState->isBipolar (sourceID, ParamIDs::hpfFrequency);

    // Convert modulated normalized cutoff to frequency
    float modNormCutoff = juce::jlimit (0.0f, 1.0f, normalizedCutoff + depth);
    float modFreq = cutoffParam->getNormalisableRange().convertFrom0to1 (modNormCutoff);

    drawHPFCurveAt (g, modFreq, getModColor (sourceID).withAlpha (Style::alphaMod), 2.0f);

    if (bipolar)
    {
        float ghostNormCutoff = juce::jlimit (0.0f, 1.0f, normalizedCutoff - depth);
        float ghostFreq = cutoffParam->getNormalisableRange().convertFrom0to1 (ghostNormCutoff);
        drawHPFCurveAt (g, ghostFreq, getModColor (sourceID).withAlpha (Style::alphaModGhost), 1.5f);
    }
}

double HPFDisplay::displayXToFreq (double x) const
{
    return kDisplayMinFreq * std::pow (kDisplayMaxFreq / kDisplayMinFreq, x);
}

double HPFDisplay::freqToDisplayX (double freq) const
{
    if (freq <= 0.0)
        return 0.0;
    return std::log (freq / kDisplayMinFreq) / std::log (kDisplayMaxFreq / kDisplayMinFreq);
}

void HPFDisplay::modulationModeChanged (ModulationModeState::Mode)
{
    repaint();
}

void HPFDisplay::targetSourceChanged (const juce::String&)
{
    repaint();
}
