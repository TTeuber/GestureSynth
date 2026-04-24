#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Theme.h"
#include "PaintHelpers.h"
#include "HoverAnimator.h"

class DelaySyncToggleComponent final : public juce::Component,
                                        private juce::AudioProcessorParameter::Listener
{
public:
    DelaySyncToggleComponent (juce::AudioParameterBool* param,
        juce::AudioParameterBool* activeParam = nullptr)
        : param (param), activeParam (activeParam)
    {
        jassert (param != nullptr);
        param->addListener (this);
        syncActive = param->get();

        if (activeParam != nullptr)
        {
            activeParam->addListener (this);
            isActive = activeParam->get();
        }
    }

    ~DelaySyncToggleComponent() override
    {
        if (param != nullptr)
            param->removeListener (this);
        if (activeParam != nullptr)
            activeParam->removeListener (this);
    }

    void setDrawOuterBox (bool shouldDraw)
    {
        if (drawOuterBox != shouldDraw)
        {
            drawOuterBox = shouldDraw;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        const int labelH = static_cast<int> (Style::labelHeight);
        constexpr int outerPad = 4;

        auto outerBounds = getLocalBounds();

        if (drawOuterBox)
        {
            // Fill with parent background
            g.fillAll (PRIMARY_COLOR);

            // Outer box
            PaintHelpers::drawComponentBox (g, outerBounds.toFloat());
        }

        // Layout: top label, inner box extends to bottom
        auto innerArea = outerBounds.reduced (outerPad);
        auto topLabelArea = innerArea.removeFromTop (labelH);
        auto innerBoxBounds = innerArea.reduced (outerPad);

        // Inner box
        PaintHelpers::drawInnerBox (g, innerBoxBounds.toFloat());

        // Visualization bounds
        const auto vizBounds = innerBoxBounds.reduced (0, 6);

        g.setColour (isActive ? TEXT_COLOR : TEXT_INACTIVE_COLOR);
        g.setFont (Style::fontComponent);

        const float activeAlpha = isActive ? 1.0f : Style::alphaInactive;

        // Draw label "Sync" (fades out on hover)
        if (hoverAnimator.getAlpha() < 0.99f)
        {
            g.setOpacity ((1.0f - hoverAnimator.getAlpha()) * activeAlpha);
            g.drawText ("Sync", topLabelArea, juce::Justification::centred, true);
        }

        // Draw value "Free" or "BPM" (fades in on hover)
        if (hoverAnimator.getAlpha() > 0.01f)
        {
            g.setOpacity (hoverAnimator.getAlpha() * activeAlpha);
            g.drawText (syncActive ? "BPM" : "Free", topLabelArea, juce::Justification::centred, true);
        }

        g.setOpacity (activeAlpha);

        // Clip visualization to inner box
        g.saveState();
        g.reduceClipRegion (innerBoxBounds);

        if (syncActive)
            drawEighthNotes (g, vizBounds);
        else
            drawClock (g, vizBounds);

        g.restoreState();
    }

    void resized() override
    {
        const int size = juce::jmin (getWidth(), getHeight());
        setBounds (getX(), getY(), size, size);
    }

    void mouseEnter (const juce::MouseEvent&) override
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        hoverAnimator.setHovered (true);
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        setMouseCursor (juce::MouseCursor::NormalCursor);
        hoverAnimator.setHovered (false);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (! getLocalBounds().contains (e.x, e.y))
            return;

        if (e.mods.isPopupMenu())
        {
            if (activeParam != nullptr)
            {
                activeParam->beginChangeGesture();
                activeParam->setValueNotifyingHost (isActive ? 0.0f : 1.0f);
                activeParam->endChangeGesture();
            }
            return;
        }

        if (param != nullptr && isActive)
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost (syncActive ? 0.0f : 1.0f);
            param->endChangeGesture();
        }
    }

private:
    void parameterValueChanged (int parameterIndex, float) override
    {
        if (param != nullptr && parameterIndex == param->getParameterIndex())
            syncActive = param->get();
        else if (activeParam != nullptr && parameterIndex == activeParam->getParameterIndex())
            isActive = activeParam->get();
        juce::MessageManager::callAsync ([this] { repaint(); });
    }

    void parameterGestureChanged (int, bool) override {}

    void drawClock (juce::Graphics& g, const juce::Rectangle<int>& bounds) const
    {
        const float cx = bounds.getCentreX();
        const float cy = bounds.getCentreY();
        const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.35f;

        g.setColour (isActive ? TEXT_COLOR : TEXT_INACTIVE_COLOR);

        // Circle outline
        g.drawEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.5f);

        // Hour hand (~10 o'clock = 300 degrees = -60 degrees from 12)
        const float hourAngle = -juce::MathConstants<float>::pi / 3.0f;
        const float hourLen = radius * 0.5f;
        g.drawLine (cx, cy,
            cx + std::sin (hourAngle) * hourLen,
            cy - std::cos (hourAngle) * hourLen,
            1.5f);

        // Minute hand (~2 o'clock)
        const float minAngle = 1.05f;
        const float minLen = radius * 0.75f;
        g.drawLine (cx, cy,
            cx + std::sin (minAngle) * minLen,
            cy - std::cos (minAngle) * minLen,
            1.5f);
    }

    void drawEighthNotes (juce::Graphics& g, const juce::Rectangle<int>& bounds) const
    {
        const float w = static_cast<float> (bounds.getWidth());
        const float h = static_cast<float> (bounds.getHeight());
        const float cx = bounds.getCentreX();
        const float cy = bounds.getCentreY();

        // Scale everything relative to available space
        const float scale = juce::jmin (w, h) * 0.4f;

        // Note head dimensions
        const float headW = scale * 0.45f;
        const float headH = scale * 0.3f;
        const float stemH = scale * 1.1f;

        // Two notes spaced apart, right note higher
        const float spacing = scale * 0.55f;
        const float noteOffset = scale * 0.3f; // vertical offset between notes

        // Center the whole figure: total height = stemH + noteOffset + headH/2
        const float totalH = stemH + noteOffset;
        const float leftNoteY = cy + totalH * 0.5f;         // left notehead (lower)
        const float rightNoteY = leftNoteY - noteOffset;     // right notehead (higher)

        g.setColour (isActive ? TEXT_COLOR : TEXT_INACTIVE_COLOR);

        // Left note
        const float lx = cx - spacing;
        juce::Path leftHead;
        leftHead.addEllipse (lx - headW * 0.5f, leftNoteY - headH * 0.5f, headW, headH);
        g.fillPath (leftHead, juce::AffineTransform::rotation (-0.3f, lx, leftNoteY));

        // Left stem
        const float lStemX = lx + headW * 0.4f;
        const float lStemTop = leftNoteY - stemH;
        g.drawLine (lStemX, leftNoteY, lStemX, lStemTop, 1.5f);

        // Right note (higher)
        const float rx = cx + spacing;
        juce::Path rightHead;
        rightHead.addEllipse (rx - headW * 0.5f, rightNoteY - headH * 0.5f, headW, headH);
        g.fillPath (rightHead, juce::AffineTransform::rotation (-0.3f, rx, rightNoteY));

        // Right stem
        const float rStemX = rx + headW * 0.4f;
        const float rStemTop = rightNoteY - stemH;
        g.drawLine (rStemX, rightNoteY, rStemX, rStemTop, 1.5f);

        // Beam connecting tops of stems (angled)
        g.drawLine (lStemX, lStemTop, rStemX, rStemTop, 2.5f);
    }

    juce::AudioParameterBool* param = nullptr;
    juce::AudioParameterBool* activeParam = nullptr;
    bool syncActive = false;
    bool isActive = true;
    bool drawOuterBox = true;
    HoverAnimator hoverAnimator { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelaySyncToggleComponent)
};
