//
// Created by Tyler Teuber on 4/27/25.
//

#include "FilterDisplay.h"
FilterDisplay::FilterDisplay (juce::AudioProcessorValueTreeState& apvts,
    juce::UndoManager* undoManager,
    std::atomic<int>* gestureCount,
    std::atomic<float>* modCutoffOutput,
    std::atomic<float>* modResonanceOutput)
    : apvts (apvts), undoManager (undoManager), gestureCount (gestureCount),
      modCutoffOutput (modCutoffOutput), modResonanceOutput (modResonanceOutput)
{
    // Add listeners to the parameters
    this->apvts.addParameterListener ("filterFrequency", this);
    this->apvts.addParameterListener ("filterResonance", this);
    this->apvts.addParameterListener ("filterOn", this);

    cutoffParam = apvts.getParameter ("filterFrequency");
    resonanceParam = apvts.getParameter ("filterResonance");

    // Initialize filterEnabled based on the actual parameter value
    auto* filterOnParam = apvts.getParameter ("filterOn");
    if (filterOnParam != nullptr)
        filterEnabled = filterOnParam->getValue() > 0.5f;

    // Get initial parameter values
    updateParameterValues();

    if (modCutoffOutput != nullptr || modResonanceOutput != nullptr)
        startTimerHz (30);

    setSize (300, 200);
}
FilterDisplay::~FilterDisplay()
{
    stopTimer();
    // Remove listeners
    apvts.removeParameterListener ("filterFrequency", this);
    apvts.removeParameterListener ("filterResonance", this);
    apvts.removeParameterListener ("filterOn", this);
}

void FilterDisplay::paint (juce::Graphics& g)
{
    // Fill background
    g.fillAll (SECONDARY_COLOR);

    // Draw control point
    drawControlPoint (g);

    // Draw parameter values
    drawParameterValues (g);

    // Draw the modulated ghost curve behind the main curve
    drawModulatedFrequencyPath (g);

    // Draw the filter response curve
    drawFrequencyPath (g);
}
void FilterDisplay::resized()
{
    // Update calculations for a new size
    updateControlPointPosition();
}

void FilterDisplay::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown())
    {
        auto* filterOnParam = apvts.getParameter ("filterOn");
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
    // Check if the mouse is near the control point
    if (isMouseOverControlPoint (e.getPosition()))
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
    }
}
void FilterDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (isDragging)
    {
        // Get the current position in the component
        const juce::Point<float> pos = e.position;

        // Check if shift is held for fine control
        const bool fineControl = e.mods.isShiftDown();

        // Calculate new parameter values
        float newCutoff, newResonance;

        if (fineControl)
        {
            // Apply fine control (reduced sensitivity)
            constexpr float sensitivity = 0.1f; // 10% of normal sensitivity
            const float xOffset = (pos.x - dragStartPosition.x) * sensitivity;
            const float yOffset = (pos.y - dragStartPosition.y) * sensitivity;

            newCutoff = juce::jlimit (0.0f, 1.0f, dragStartCutoff + xOffset / getWidth());
            newResonance = juce::jlimit (0.0f, 1.0f, dragStartResonance - yOffset / getHeight());
        }
        else
        {
            // Direct control - map position directly to parameter values
            newCutoff = juce::jlimit (0.0f, 1.0f, pos.x / getWidth());
            newResonance = juce::jlimit (0.0f, 1.0f, 1.0f - pos.y / getHeight());
        }

        if (cutoffParam && resonanceParam)
        {
            cutoffParam->setValueNotifyingHost (newCutoff);
            resonanceParam->setValueNotifyingHost (newResonance);
        }

        // Update the display
        repaint();
    }
}
void FilterDisplay::mouseUp (const juce::MouseEvent& e)
{
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
    // Called when parameters change from outside this component
    if (parameterID == "filterFrequency" || parameterID == "filterResonance")
    {
        updateParameterValues();

        // Update UI on the message thread
        juce::MessageManager::callAsync ([this] {
            updateControlPointPosition();
            repaint();
        });
    }
    else if (parameterID == "filterOn")
    {
        filterEnabled = newValue > 0.5f;
    }
}
void FilterDisplay::updateParameterValues()
{
    // Get the normalized values (0-1)
    const auto* cutoffParam = apvts.getParameter ("filterFrequency");
    auto* resonanceParam = apvts.getParameter ("filterResonance");

    if (cutoffParam && resonanceParam)
    {
        normalizedCutoff = cutoffParam->getValue();
        normalizedResonance = resonanceParam->getValue();

        cutoffFrequency = cutoffParam->getNormalisableRange().convertFrom0to1 (normalizedCutoff);
        resonance = resonanceParam->getNormalisableRange().convertFrom0to1 (normalizedResonance);
    }

    updateControlPointPosition();
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
    // Map x (0 to 1) to frequency (20 Hz to 20 kHz, logarithmic scale)
    const double freq = cutoffParam->convertFrom0to1 (x);

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
    return 20.0 * std::log10 (gain); // Convert to herself
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

void FilterDisplay::updateControlPointPosition()
{
    // Convert normalized values to component coordinates
    controlPoint.x = normalizedCutoff * static_cast<float> (getWidth());
    controlPoint.y = (1.0f - normalizedResonance) * static_cast<float> (getHeight());
}
bool FilterDisplay::isMouseOverControlPoint (const juce::Point<int>& mousePosition) const
{
    // Calculate distance from mouse to control point
    const float distance = mousePosition.getDistanceFrom (juce::Point<int> (
        static_cast<int> (controlPoint.x),
        static_cast<int> (controlPoint.y)));

    return distance <= controlPointRadius * 1.5f; // slightly larger than the visual radius for easier grabbing
}
void FilterDisplay::drawControlPoint (juce::Graphics& g) const
{
    // Draw outer circle
    g.setColour (filterEnabled ? juce::Colours::white : juce::Colours::grey);
    g.drawEllipse (controlPoint.x - controlPointRadius,
        controlPoint.y - controlPointRadius,
        controlPointRadius * 2.0f,
        controlPointRadius * 2.0f,
        2.0f);

    // Draw the inner circle
    g.setColour (filterEnabled ? (isDragging ? juce::Colours::orange : juce::Colours::grey) : juce::Colours::black);
    g.fillEllipse (controlPoint.x - (controlPointRadius - 2.0f),
        controlPoint.y - (controlPointRadius - 2.0f),
        (controlPointRadius - 2.0f) * 2.0f,
        (controlPointRadius - 2.0f) * 2.0f);
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

    // Draw a shift key hint when dragging
    if (isDragging)
    {
        g.setFont (12.0f);
        g.setColour (juce::Colours::lightgrey);
        g.drawText ("Hold Shift for fine control", 5, getHeight() - 40, getWidth() - 10, 20, juce::Justification::bottomLeft, true);
    }
}

void FilterDisplay::timerCallback()
{
    if (modCutoffOutput != nullptr)
        modulatedNormCutoff = modCutoffOutput->load (std::memory_order_relaxed);
    if (modResonanceOutput != nullptr)
        modulatedNormResonance = modResonanceOutput->load (std::memory_order_relaxed);

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

    if (cutoffParam == nullptr || resonanceParam == nullptr)
        return;

    // Convert normalized modulated values to actual frequency/resonance
    const float modCutoffFreq = cutoffParam->getNormalisableRange().convertFrom0to1 (modulatedNormCutoff);
    const float modRes = resonanceParam->getNormalisableRange().convertFrom0to1 (modulatedNormResonance);

    if (modCutoffFreq <= 0.0f || modRes <= 0.0f)
        return;

    juce::Path path;
    constexpr int step = 5;

    for (int xPixel = 0; xPixel < getWidth(); xPixel += step)
    {
        const double x = static_cast<double> (xPixel) / getWidth();
        const double freq = cutoffParam->convertFrom0to1 (static_cast<float> (x));

        // Same math as getYCoordinate but with modulated cutoff/resonance
        const double dB = computeSecondOrderStage (freq, modCutoffFreq, modRes);
        const float yPixel = getHeight() / 2.0f - static_cast<float> (dB) * getHeight() / 50.0f;

        if (xPixel == 0)
            path.startNewSubPath (static_cast<float> (xPixel), yPixel);
        else
            path.lineTo (static_cast<float> (xPixel), yPixel);
    }

    g.setColour (TEXT_COLOR.withAlpha (0.25f));
    g.strokePath (path, juce::PathStrokeType (1.5f));
}