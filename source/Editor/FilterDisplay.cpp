//
// Created by Tyler Teuber on 4/27/25.
//

#include "FilterDisplay.h"
#include "../Utility/AtomicHelpers.h"
#include "../Utility/Parameters.h"
FilterDisplay::FilterDisplay (juce::AudioProcessorValueTreeState& apvts,
    const UIContext& ctx,
    std::atomic<float>* modCutoffOutput,
    std::atomic<float>* modResonanceOutput)
    : apvts (apvts), undoManager (ctx.undoManager), gestureCount (ctx.gestureCount),
      modCutoffOutput (modCutoffOutput), modResonanceOutput (modResonanceOutput),
      animSource (ctx.animationSource), modModeState (ctx.modModeState)
{
    // Add listeners to the parameters
    this->apvts.addParameterListener (ParamIDs::filterFrequency, this);
    this->apvts.addParameterListener (ParamIDs::filterResonance, this);
    this->apvts.addParameterListener (ParamIDs::filterOn, this);

    cutoffParam = apvts.getParameter (ParamIDs::filterFrequency);
    resonanceParam = apvts.getParameter (ParamIDs::filterResonance);

    // Initialize filterEnabled based on the actual parameter value
    auto* filterOnParam = apvts.getParameter (ParamIDs::filterOn);
    if (filterOnParam != nullptr)
        filterEnabled = filterOnParam->getValue() > 0.5f;

    // Get initial parameter values
    updateParameterValues();

    if ((modCutoffOutput != nullptr || modResonanceOutput != nullptr) && animSource != nullptr)
        animSource->addListener (this, AnimationFrameSource::Rate::Hz30);

    setSize (300, 200);
}
FilterDisplay::~FilterDisplay()
{
    if (animSource != nullptr)
        animSource->removeListener (this);
    // Remove listeners
    apvts.removeParameterListener (ParamIDs::filterFrequency, this);
    apvts.removeParameterListener (ParamIDs::filterResonance, this);
    apvts.removeParameterListener (ParamIDs::filterOn, this);
}

void FilterDisplay::paint (juce::Graphics& g)
{
    // Fill background
    g.fillAll (SECONDARY_COLOR);

    // Draw parameter values
    drawParameterValues (g);

    // Draw the modulated ghost curve behind the main curve
    drawModulatedFrequencyPath (g);

    // Draw the filter response curve
    drawFrequencyPath (g);

    // Draw mod mode overlay
    drawModModeOverlay (g);
}
void FilterDisplay::resized()
{
}

void FilterDisplay::mouseDown (const juce::MouseEvent& e)
{
    // Modulation context menu on right-click in mod mode
    if (e.mods.isRightButtonDown()
        && modModeState != nullptr && modModeState->isModulationMode()
        && filterEnabled)
    {
        auto sourceID = modModeState->getTargetSourceID();
        std::vector<ModulationContextMenu::ParamInfo> entries;
        if (modModeState->findSlotIndex (sourceID, ParamIDs::filterFrequency) >= 0)
            entries.push_back ({ cutoffParam->getName (15), sourceID, ParamIDs::filterFrequency });
        if (modModeState->findSlotIndex (sourceID, ParamIDs::filterResonance) >= 0)
            entries.push_back ({ resonanceParam->getName (15), sourceID, ParamIDs::filterResonance });
        if (!entries.empty())
        {
            showModulationContextMenu (this, modModeState, std::move (entries), e.getScreenPosition());
            return;
        }
    }

    if (e.mods.isRightButtonDown())
    {
        auto* filterOnParam = apvts.getParameter (ParamIDs::filterOn);
        if (filterOnParam != nullptr)
        {
            if (undoManager != nullptr)
                undoManager->beginNewTransaction();
            filterOnParam->beginChangeGesture();
            filterOnParam->setValueNotifyingHost (!filterEnabled);
            filterOnParam->endChangeGesture();

            repaint();
        }
    }

    // Modulation mode handling
    if (modModeState != nullptr && modModeState->isModulationMode())
    {
        if (e.mods.isShiftDown())
        {
            if (!filterEnabled)
                return;

            isDragging = true;
            dragStartPosition = e.position;
            dragStartCutoff = normalizedCutoff;
            dragStartResonance = normalizedResonance;

            if (undoManager != nullptr)
                undoManager->beginNewTransaction();
            if (cutoffParam)
                cutoffParam->beginChangeGesture();
            if (resonanceParam)
                resonanceParam->beginChangeGesture();
            if (gestureCount != nullptr)
                ++(*gestureCount);
            cutoffEngaged = false;
            resonanceEngaged = false;
            return;
        }

        if (undoManager != nullptr)
            undoManager->beginNewTransaction();
        if (gestureCount != nullptr)
            ++(*gestureCount);

        modDragStartX = e.x;
        modDragStartY = e.y;
        isModDragging = true;
        modCutoffEngaged = false;
        modResonanceEngaged = false;
        return;
    }

    if (filterEnabled)
    {
        isDragging = true;
        dragStartPosition = e.position;
        dragStartCutoff = normalizedCutoff;
        dragStartResonance = normalizedResonance;

        if (undoManager != nullptr)
            undoManager->beginNewTransaction();
        if (cutoffParam)
            cutoffParam->beginChangeGesture();
        if (resonanceParam)
            resonanceParam->beginChangeGesture();
        if (gestureCount != nullptr)
            ++(*gestureCount);
        cutoffEngaged = false;
        resonanceEngaged = false;
    }
}
void FilterDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (isModDragging && modModeState != nullptr)
    {
        auto sourceID = modModeState->getTargetSourceID();
        const int absX = std::abs (e.x - modDragStartX);
        const int absY = std::abs (e.y - modDragStartY);

        const bool wasModCutoffEngaged = modCutoffEngaged;
        const bool wasModResonanceEngaged = modResonanceEngaged;
        const bool wasModEitherEngaged = wasModCutoffEngaged || wasModResonanceEngaged;

        if (!wasModCutoffEngaged
            && absX >= (wasModEitherEngaged ? kSecondaryThreshold : kPrimaryThreshold))
        {
            int slot = modModeState->getOrCreateSlot (sourceID, ParamIDs::filterFrequency);
            if (slot >= 0)
            {
                modCutoffEngaged = true;
                modCutoffRefX = e.x;
                modDragInitialCutoffDepth = modModeState->getDepth (sourceID, ParamIDs::filterFrequency);
            }
        }
        if (!wasModResonanceEngaged
            && absY >= (wasModEitherEngaged ? kSecondaryThreshold : kPrimaryThreshold))
        {
            int slot = modModeState->getOrCreateSlot (sourceID, ParamIDs::filterResonance);
            if (slot >= 0)
            {
                modResonanceEngaged = true;
                modResonanceRefY = e.y;
                modDragInitialResDepth = modModeState->getDepth (sourceID, ParamIDs::filterResonance);
            }
        }

        if (modCutoffEngaged)
        {
            float hDelta = static_cast<float> (e.x - modCutoffRefX) / static_cast<float> (getWidth());
            float newCutoffDepth = juce::jlimit (-1.0f, 1.0f, modDragInitialCutoffDepth + hDelta);
            modModeState->setDepth (sourceID, ParamIDs::filterFrequency, newCutoffDepth);
        }
        if (modResonanceEngaged)
        {
            float vDelta = static_cast<float> (modResonanceRefY - e.y) / static_cast<float> (getHeight());
            float newResDepth = juce::jlimit (-1.0f, 1.0f, modDragInitialResDepth + vDelta);
            modModeState->setDepth (sourceID, ParamIDs::filterResonance, newResDepth);
        }

        repaint();
        return;
    }

    if (isDragging)
    {
        const int absX = std::abs (static_cast<int> (e.position.x - dragStartPosition.x));
        const int absY = std::abs (static_cast<int> (e.position.y - dragStartPosition.y));

        const bool wasCutoffEngaged = cutoffEngaged;
        const bool wasResonanceEngaged = resonanceEngaged;
        const bool wasEitherEngaged = wasCutoffEngaged || wasResonanceEngaged;

        if (!wasCutoffEngaged && absX >= (wasEitherEngaged ? kSecondaryThreshold : kPrimaryThreshold))
        {
            cutoffEngaged = true;
            cutoffRefX = static_cast<int> (e.position.x);
            initialCutoffValue = normalizedCutoff;
        }
        if (!wasResonanceEngaged && absY >= (wasEitherEngaged ? kSecondaryThreshold : kPrimaryThreshold))
        {
            resonanceEngaged = true;
            resonanceRefY = static_cast<int> (e.position.y);
            initialResonanceValue = normalizedResonance;
        }

        if (cutoffEngaged && cutoffParam)
        {
            float horizontalDelta = (e.position.x - static_cast<float> (cutoffRefX)) / static_cast<float> (getWidth());
            cutoffParam->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, initialCutoffValue + horizontalDelta));
        }
        if (resonanceEngaged && resonanceParam)
        {
            float verticalDelta = (static_cast<float> (resonanceRefY) - e.position.y) / static_cast<float> (getHeight());
            resonanceParam->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, initialResonanceValue + verticalDelta));
        }

        repaint();
    }
}
void FilterDisplay::mouseUp (const juce::MouseEvent& e)
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
        if (resonanceParam)
            resonanceParam->endChangeGesture();
        if (gestureCount != nullptr)
            --(*gestureCount);
    }
    isDragging = false;
}

void FilterDisplay::parameterChanged (const juce::String& parameterID, float newValue)
{
    juce::ignoreUnused (newValue);
    // Called when parameters change from outside this component
    if (parameterID == ParamIDs::filterFrequency || parameterID == ParamIDs::filterResonance)
    {
        updateParameterValues();

        // Update UI on the message thread
        juce::MessageManager::callAsync ([this] {
            repaint();
        });
    }
    else if (parameterID == ParamIDs::filterOn)
    {
        filterEnabled = newValue > 0.5f;
    }
}
void FilterDisplay::updateParameterValues()
{
    // Get the normalized values (0-1)
    const auto* cutoffParam = apvts.getParameter (ParamIDs::filterFrequency);
    auto* resonanceParam = apvts.getParameter (ParamIDs::filterResonance);

    if (cutoffParam && resonanceParam)
    {
        normalizedCutoff = cutoffParam->getValue();
        normalizedResonance = resonanceParam->getValue();

        cutoffFrequency = cutoffParam->getNormalisableRange().convertFrom0to1 (normalizedCutoff);
        resonance = resonanceParam->getNormalisableRange().convertFrom0to1 (normalizedResonance);
    }

}

void FilterDisplay::drawFrequencyPath (juce::Graphics& g) const
{
    juce::Path path;
    constexpr int step = 5;

    for (int xPixel = 0; xPixel < getWidth(); xPixel += step)
    {
        const double x = static_cast<double> (xPixel) / getWidth(); // Normalize to 0-1
        const double dB = getYCoordinate (x);
        // Map dB to y-pixel (invert and scale, e.g., 0 dB at the middle of height)
        const float yPixel = getHeight() / 2.0f - dB * getHeight() / 50.0f; // Example scaling
        if (xPixel == 0)
        {
            path.startNewSubPath (xPixel, yPixel);
        }
        else
        {
            path.lineTo (xPixel, yPixel);
        }
    }

    g.setColour (filterEnabled ? TEXT_COLOR : TEXT_INACTIVE_COLOR);
    g.strokePath (path, juce::PathStrokeType (2.0f));
}

double FilterDisplay::getYCoordinate (const float x, const int order) const
{
    // Map x (0 to 1) to frequency using true logarithmic scale
    const double freq = displayXToFreq (x);

    // Handle invalid inputs
    if (cutoffFrequency <= 0.0 || resonance <= 0.0 || order < 1 || freq <= 0.0)
    {
        return -60.0; // Return a low dB value for invalid cases
    }

    // Calculate total gain in dB by summing stages
    double totalGainDb = 0.0;
    const int numSecondOrderStages = order / 2; // Number of second-order stages
    const bool hasFirstOrderStage = order % 2 == 1; // True if the order is odd

    // Process second-order stages
    for (int i = 0; i < numSecondOrderStages; ++i)
    {
        totalGainDb += computeSecondOrderStage (freq, cutoffFrequency, resonance);
    }

    // Process the first-order stage if the order is odd
    if (hasFirstOrderStage)
    {
        totalGainDb += computeFirstOrderStage (freq, cutoffFrequency);
    }

    return totalGainDb;
}
double FilterDisplay::computeSecondOrderStage (const double freq, const double cutoffFreq, const double q)
{
    const double w = freq / cutoffFreq; // Normalized frequency
    const double wSquared = w * w;
    const double denominator = std::sqrt ((1.0 - wSquared) * (1.0 - wSquared) + w / q * (w / q));

    // Avoid division by zero
    if (denominator < 1e-10)
    {
        return -60.0; // Return a low dB value for numerical stability
    }

    const double gain = 1.0 / denominator;
    return 20.0 * std::log10 (gain); // Convert to dB
}
double FilterDisplay::computeFirstOrderStage (const double freq, const double cutoffFreq)
{
    const double w = freq / cutoffFreq; // Normalized frequency
    const double denominator = std::sqrt (1.0 + w * w);

    // Avoid division by zero
    if (denominator < 1e-10)
    {
        return -60.0; // Return a low dB value for numerical stability
    }

    const double gain = 1.0 / denominator;
    return 20.0 * std::log10 (gain); // Convert to dB
}


void FilterDisplay::drawParameterValues (juce::Graphics& g) const
{
    g.setColour (filterEnabled ? TEXT_COLOR : TEXT_INACTIVE_COLOR);
    g.setFont (14.0f);

    // Format the frequency value
    juce::String freqText;
    if (cutoffFrequency < 1000.0f)
        freqText = juce::String (static_cast<int> (cutoffFrequency)) + " Hz";
    else
        freqText = juce::String (cutoffFrequency / 1000.0f, 1) + " kHz";

    // Draw frequency text
    g.drawText ("Frequency: " + freqText, 5, getHeight() - 25, getWidth() / 2 - 10, 20, juce::Justification::bottomLeft, true);

    // Draw resonance text
    g.drawText ("Resonance: " + juce::String (resonance, 2), getWidth() / 2 + 5, 5, getWidth() / 2 - 10, 20, juce::Justification::topRight, true);

}

void FilterDisplay::onAnimationFrame()
{
    constexpr float alpha = 0.3f;

    if (modCutoffOutput != nullptr)
    {
        float target = AtomicHelpers::paramLoad (*modCutoffOutput);
        modulatedNormCutoff += alpha * (target - modulatedNormCutoff);
    }
    if (modResonanceOutput != nullptr)
    {
        float target = AtomicHelpers::paramLoad (*modResonanceOutput);
        modulatedNormResonance += alpha * (target - modulatedNormResonance);
    }

    repaint();
}

void FilterDisplay::drawModulatedFrequencyPath (juce::Graphics& g) const
{
    if (modCutoffOutput == nullptr && modResonanceOutput == nullptr)
        return;

    // Skip if modulated values are close to the base values
    if (std::abs (modulatedNormCutoff - normalizedCutoff) < 0.001f
        && std::abs (modulatedNormResonance - normalizedResonance) < 0.001f)
        return;

    drawFilterCurveAt (g, modulatedNormCutoff, modulatedNormResonance,
        TEXT_COLOR.withAlpha (0.25f), 1.5f);
}

void FilterDisplay::drawFilterCurveAt (juce::Graphics& g, float normCutoff, float normRes,
    juce::Colour colour, float strokeWidth) const
{
    if (cutoffParam == nullptr || resonanceParam == nullptr)
        return;

    const float modCutoffFreq = cutoffParam->getNormalisableRange().convertFrom0to1 (normCutoff);
    const float modRes = resonanceParam->getNormalisableRange().convertFrom0to1 (normRes);

    if (modCutoffFreq <= 0.0f || modRes <= 0.0f)
        return;

    juce::Path path;
    constexpr int step = 5;

    for (int xPixel = 0; xPixel < getWidth(); xPixel += step)
    {
        const double x = static_cast<double> (xPixel) / getWidth();
        const double freq = displayXToFreq (x);

        const double dB = computeSecondOrderStage (freq, modCutoffFreq, modRes);
        const float yPixel = getHeight() / 2.0f - static_cast<float> (dB) * getHeight() / 50.0f;

        if (xPixel == 0)
            path.startNewSubPath (static_cast<float> (xPixel), yPixel);
        else
            path.lineTo (static_cast<float> (xPixel), yPixel);
    }

    g.setColour (colour);
    g.strokePath (path, juce::PathStrokeType (strokeWidth));
}

void FilterDisplay::drawModModeOverlay (juce::Graphics& g) const
{
    if (modModeState == nullptr || !modModeState->isModulationMode())
        return;

    auto sourceID = modModeState->getTargetSourceID();

    float cutoffDepth = modModeState->getDepth (sourceID, ParamIDs::filterFrequency);
    float resDepth = modModeState->getDepth (sourceID, ParamIDs::filterResonance);


    bool bipolarCutoff = modModeState->isBipolar (sourceID, ParamIDs::filterFrequency);
    bool bipolarRes = modModeState->isBipolar (sourceID, ParamIDs::filterResonance);

    // Draw cyan curve at modulated position
    float modCutoff = juce::jlimit (0.0f, 1.0f, normalizedCutoff + cutoffDepth);
    float modRes = juce::jlimit (0.0f, 1.0f, normalizedResonance + resDepth);
    drawFilterCurveAt (g, modCutoff, modRes, MOD_COLOR.withAlpha (0.7f), 2.0f);

    // Draw faint ghost for bipolar
    if (bipolarCutoff || bipolarRes)
    {
        float ghostCutoff = bipolarCutoff ? juce::jlimit (0.0f, 1.0f, normalizedCutoff - cutoffDepth) : modCutoff;
        float ghostRes = bipolarRes ? juce::jlimit (0.0f, 1.0f, normalizedResonance - resDepth) : modRes;
        drawFilterCurveAt (g, ghostCutoff, ghostRes, MOD_COLOR.withAlpha (0.2f), 1.5f);
    }
}

double FilterDisplay::displayXToFreq (double x) const
{
    return kDisplayMinFreq * std::pow (kDisplayMaxFreq / kDisplayMinFreq, x);
}

double FilterDisplay::freqToDisplayX (double freq) const
{
    if (freq <= 0.0)
        return 0.0;
    return std::log (freq / kDisplayMinFreq) / std::log (kDisplayMaxFreq / kDisplayMinFreq);
}
