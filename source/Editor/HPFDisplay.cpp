//
// High Pass Filter Display Component
//

#include "HPFDisplay.h"

HPFDisplay::HPFDisplay (juce::AudioProcessorValueTreeState& apvts) : apvts (apvts)
{
    this->apvts.addParameterListener ("hpfFrequency", this);
    this->apvts.addParameterListener ("hpfOn", this);

    cutoffParam = apvts.getParameter ("hpfFrequency");

    auto* hpfOnParam = apvts.getParameter ("hpfOn");
    if (hpfOnParam != nullptr)
        hpfEnabled = hpfOnParam->getValue() > 0.5f;

    updateParameterValues();

    setSize (200, 150);
}

HPFDisplay::~HPFDisplay()
{
    apvts.removeParameterListener ("hpfFrequency", this);
    apvts.removeParameterListener ("hpfOn", this);
}

void HPFDisplay::paint (juce::Graphics& g)
{
    g.fillAll (SECONDARY_COLOR);

    drawControlPoint (g);
    drawParameterValues (g);
    drawFrequencyPath (g);
}

void HPFDisplay::resized()
{
    updateControlPointPosition();
}

void HPFDisplay::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown())
    {
        auto* hpfOnParam = apvts.getParameter ("hpfOn");
        if (hpfOnParam != nullptr)
        {
            hpfOnParam->beginChangeGesture();
            hpfOnParam->setValueNotifyingHost (!hpfEnabled);
            hpfOnParam->endChangeGesture();
            repaint();
        }
    }

    if (isMouseOverControlPoint (e.getPosition()))
    {
        isDragging = true;
        dragStartPosition = e.position;
        dragStartDisplayX = static_cast<float> (freqToDisplayX (cutoffFrequency));
    }
}

void HPFDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (isDragging)
    {
        const juce::Point<float> pos = e.position;
        const bool fineControl = e.mods.isShiftDown();

        float newCutoff;

        if (fineControl)
        {
            constexpr float sensitivity = 0.1f;
            const float xOffset = (pos.x - dragStartPosition.x) * sensitivity;
            const float displayX = juce::jlimit (0.0f, 1.0f, dragStartDisplayX + xOffset / getWidth());
            double freq = juce::jlimit (20.0, 3000.0, displayXToFreq (displayX));
            newCutoff = static_cast<float> (cutoffParam->getNormalisableRange().convertTo0to1 (static_cast<float> (freq)));
        }
        else
        {
            double freq = displayXToFreq (static_cast<double> (pos.x) / getWidth());
            freq = juce::jlimit (20.0, 3000.0, freq);
            newCutoff = static_cast<float> (cutoffParam->getNormalisableRange().convertTo0to1 (static_cast<float> (freq)));
        }

        if (cutoffParam)
            cutoffParam->setValueNotifyingHost (newCutoff);

        repaint();
    }
}

void HPFDisplay::mouseUp (const juce::MouseEvent& e)
{
    juce::ignoreUnused (e);
    isDragging = false;
}

void HPFDisplay::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == "hpfFrequency")
    {
        updateParameterValues();

        juce::MessageManager::callAsync ([this] {
            updateControlPointPosition();
            repaint();
        });
    }
    else if (parameterID == "hpfOn")
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

    updateControlPointPosition();
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
    if (cutoffFrequency <= 0.0 || freq <= 0.0)
        return -60.0;

    const double w = freq / cutoffFrequency;
    const double wSquared = w * w;
    const double q = 1.0 / std::sqrt (2.0); // ~0.707, Butterworth
    const double denominator = std::sqrt ((1.0 - wSquared) * (1.0 - wSquared) + (w / q) * (w / q));

    if (denominator < 1e-10)
        return -60.0;

    // HP gain: H(s) = s^2 / (s^2 + s/Q + 1) => |H| = w^2 / denominator
    const double gain = wSquared / denominator;

    if (gain < 1e-10)
        return -60.0;

    return 20.0 * std::log10 (gain);
}

void HPFDisplay::updateControlPointPosition()
{
    controlPoint.x = static_cast<float> (freqToDisplayX (cutoffFrequency)) * static_cast<float> (getWidth());
    // Place control point on the curve at the cutoff frequency
    const double dBAtCutoff = getHPGainDb (cutoffFrequency);
    controlPoint.y = getHeight() / 2.0f - static_cast<float> (dBAtCutoff) * getHeight() / 50.0f;
}

bool HPFDisplay::isMouseOverControlPoint (const juce::Point<int>& mousePosition) const
{
    const float distance = mousePosition.getDistanceFrom (juce::Point<int> (
        static_cast<int> (controlPoint.x),
        static_cast<int> (controlPoint.y)));

    return distance <= controlPointRadius * 1.5f;
}

void HPFDisplay::drawControlPoint (juce::Graphics& g) const
{
    g.setColour (hpfEnabled ? juce::Colours::white : juce::Colours::grey);
    g.drawEllipse (controlPoint.x - controlPointRadius,
        controlPoint.y - controlPointRadius,
        controlPointRadius * 2.0f,
        controlPointRadius * 2.0f,
        2.0f);

    g.setColour (hpfEnabled ? (isDragging ? juce::Colours::orange : juce::Colours::grey) : juce::Colours::black);
    g.fillEllipse (controlPoint.x - (controlPointRadius - 2.0f),
        controlPoint.y - (controlPointRadius - 2.0f),
        (controlPointRadius - 2.0f) * 2.0f,
        (controlPointRadius - 2.0f) * 2.0f);
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

    if (isDragging)
    {
        g.setFont (12.0f);
        g.setColour (juce::Colours::lightgrey);
        g.drawText ("Hold Shift for fine control", 5, getHeight() - 40, getWidth() - 10, 20, juce::Justification::bottomLeft, true);
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
