//
// High Pass Filter Display Component
//

#include "HPFDisplay.h"
#include "../Utility/Parameters.h"

HPFDisplay::HPFDisplay (juce::AudioProcessorValueTreeState& apvts,
    juce::UndoManager* undoManager,
    std::atomic<int>* gestureCount,
    ModulationModeState* modModeState)
    : apvts (apvts), undoManager (undoManager), gestureCount (gestureCount),
      modModeState (modModeState)
{
    this->apvts.addParameterListener (ParamIDs::hpfFrequency, this);
    this->apvts.addParameterListener (ParamIDs::hpfOn, this);

    cutoffParam = apvts.getParameter (ParamIDs::hpfFrequency);

    auto* hpfOnParam = apvts.getParameter (ParamIDs::hpfOn);
    if (hpfOnParam != nullptr)
        hpfEnabled = hpfOnParam->getValue() > 0.5f;

    updateParameterValues();

    setSize (200, 150);
}

HPFDisplay::~HPFDisplay()
{
    apvts.removeParameterListener (ParamIDs::hpfFrequency, this);
    apvts.removeParameterListener (ParamIDs::hpfOn, this);
}

void HPFDisplay::paint (juce::Graphics& g)
{
    g.fillAll (SECONDARY_COLOR);

    drawParameterValues (g);
    drawFrequencyPath (g);
    drawModModeOverlay (g);
}

void HPFDisplay::resized()
{
}

void HPFDisplay::mouseDown (const juce::MouseEvent& e)
{
    // Modulation context menu on right-click in mod mode
    if (e.mods.isRightButtonDown()
        && modModeState != nullptr && modModeState->isModulationMode()
        && hpfEnabled)
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

    if (e.mods.isRightButtonDown())
    {
        auto* hpfOnParam = apvts.getParameter (ParamIDs::hpfOn);
        if (hpfOnParam != nullptr)
        {
            if (undoManager != nullptr)
                undoManager->beginNewTransaction();
            hpfOnParam->beginChangeGesture();
            hpfOnParam->setValueNotifyingHost (!hpfEnabled);
            hpfOnParam->endChangeGesture();
            repaint();
        }
    }

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

void HPFDisplay::mouseDrag (const juce::MouseEvent& e)
{
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
    juce::ignoreUnused (e);
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
        const float yPixel = getHeight() / 2.0f - static_cast<float> (dB) * getHeight() / 50.0f;

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
    g.setColour (hpfEnabled ? TEXT_COLOR : TEXT_INACTIVE_COLOR);
    g.setFont (14.0f);

    juce::String freqText;
    if (cutoffFrequency < 1000.0f)
        freqText = juce::String (static_cast<int> (cutoffFrequency)) + " Hz";
    else
        freqText = juce::String (cutoffFrequency / 1000.0f, 1) + " kHz";

    g.drawText ("HPF: " + freqText, 5, getHeight() - 25, getWidth() - 10, 20, juce::Justification::bottomLeft, true);
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
        const float yPixel = getHeight() / 2.0f - static_cast<float> (dB) * getHeight() / 50.0f;

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

    drawHPFCurveAt (g, modFreq, MOD_COLOR.withAlpha (0.7f), 2.0f);

    if (bipolar)
    {
        float ghostNormCutoff = juce::jlimit (0.0f, 1.0f, normalizedCutoff - depth);
        float ghostFreq = cutoffParam->getNormalisableRange().convertFrom0to1 (ghostNormCutoff);
        drawHPFCurveAt (g, ghostFreq, MOD_COLOR.withAlpha (0.2f), 1.5f);
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
