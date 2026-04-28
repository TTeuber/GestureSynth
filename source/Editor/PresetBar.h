#pragma once

#include "../PluginProcessor.h"
#include "../Theme.h"
#include "Utility/PaintHelpers.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

// =============================================================================
// PresetBar: unified preset control with three click regions inside one box
//   [floppy save]  [   preset name   ]  [<-]  [->]
// =============================================================================
class PresetBar final : public juce::Component
{
public:
    PresetBar() = default;

    void setup (PluginProcessor* processor, juce::LookAndFeel* lafForMenu)
    {
        proc = processor;
        menuLookAndFeel = lafForMenu;
    }

    void setPresetName (const juce::String& name)
    {
        presetName = name;
        repaint();
    }

    std::function<void (int)> onNavigate;
    std::function<void (const juce::File&)> onLoadFile;

    void paint (juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        // Match the tab buttons to the left: TERTIARY_COLOR fill, radiusSmall, no border.
        g.setColour (TERTIARY_COLOR);
        g.fillRoundedRectangle (bounds, Style::radiusSmall);

        // Hover backgrounds (drawn behind the icons / text)
        auto fillRegion = [&] (Region r, juce::Rectangle<float> rect)
        {
            if (hoveredRegion == r)
            {
                g.setColour (TERTIARY_COLOR.brighter (0.06f));
                g.fillRoundedRectangle (rect, Style::radiusSmall);
            }
        };
        fillRegion (Region::Save, saveRegion);
        fillRegion (Region::Name, nameRegion);
        fillRegion (Region::Prev, prevRegion);
        fillRegion (Region::Next, nextRegion);

        // Internal vertical separators (subtle, matching the tab bar palette)
        g.setColour (TEXT_COLOR.withAlpha (0.15f));
        const float top    = bounds.getY() + 5.0f;
        const float bottom = bounds.getBottom() - 5.0f;
        g.drawLine (saveRegion.getRight(), top, saveRegion.getRight(), bottom, 1.0f);
        g.drawLine (prevRegion.getX(),     top, prevRegion.getX(),     bottom, 1.0f);
        g.drawLine (nextRegion.getX(),     top, nextRegion.getX(),     bottom, 1.0f);

        // Icons + text
        drawSaveIcon (g, saveRegion);
        drawArrowIcon (g, prevRegion, true);
        drawArrowIcon (g, nextRegion, false);

        g.setColour (TEXT_COLOR);
        g.setFont (Style::fontLabel);
        const auto displayName = presetName.isEmpty() ? juce::String ("Presets") : presetName;
        g.drawText (displayName, nameRegion.toNearestInt(), juce::Justification::centred, true);
    }

    void resized() override
    {
        const auto bounds = getLocalBounds().toFloat();
        const float h = bounds.getHeight();
        const float arrowW = h;          // square arrow regions
        const float saveW  = h;          // square save region

        saveRegion = bounds.withWidth (saveW);
        nextRegion = bounds.withX (bounds.getRight() - arrowW).withWidth (arrowW);
        prevRegion = bounds.withX (nextRegion.getX() - arrowW).withWidth (arrowW);
        nameRegion = bounds.withX (saveRegion.getRight()).withWidth (prevRegion.getX() - saveRegion.getRight());
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        const auto r = regionAt (e.position);
        if (r != hoveredRegion)
        {
            hoveredRegion = r;
            repaint();
        }
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        if (hoveredRegion != Region::None)
        {
            hoveredRegion = Region::None;
            repaint();
        }
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (! getLocalBounds().contains (e.x, e.y))
            return;

        switch (regionAt (e.position))
        {
            case Region::Save: openSaveDialog(); break;
            case Region::Name: openPresetMenu(); break;
            case Region::Prev: if (onNavigate) onNavigate (-1); break;
            case Region::Next: if (onNavigate) onNavigate (1); break;
            case Region::None: break;
        }
    }

    juce::MouseCursor getMouseCursor() override
    {
        return juce::MouseCursor::PointingHandCursor;
    }

private:
    enum class Region { None, Save, Name, Prev, Next };

    Region regionAt (juce::Point<float> p) const
    {
        if (saveRegion.contains (p)) return Region::Save;
        if (nameRegion.contains (p)) return Region::Name;
        if (prevRegion.contains (p)) return Region::Prev;
        if (nextRegion.contains (p)) return Region::Next;
        return Region::None;
    }

    void drawSaveIcon (juce::Graphics& g, juce::Rectangle<float> region) const
    {
        const auto box = region.reduced (region.getWidth() * 0.25f, region.getHeight() * 0.22f);
        const float corner = 1.5f;
        const float notchW = box.getWidth() * 0.22f;
        const float shutterH = box.getHeight() * 0.32f;

        // Body outline with a corner notch (top-right) — drawn as a Path so the notch reads.
        juce::Path body;
        body.startNewSubPath (box.getX() + corner, box.getY());
        body.lineTo (box.getRight() - notchW, box.getY());
        body.lineTo (box.getRight(), box.getY() + notchW);
        body.lineTo (box.getRight(), box.getBottom() - corner);
        body.quadraticTo (box.getRight(), box.getBottom(), box.getRight() - corner, box.getBottom());
        body.lineTo (box.getX() + corner, box.getBottom());
        body.quadraticTo (box.getX(), box.getBottom(), box.getX(), box.getBottom() - corner);
        body.lineTo (box.getX(), box.getY() + corner);
        body.quadraticTo (box.getX(), box.getY(), box.getX() + corner, box.getY());

        // Shutter slot at the top
        const float shutterInset = box.getWidth() * 0.18f;
        juce::Rectangle<float> shutter (box.getX() + shutterInset,
                                        box.getY() + box.getHeight() * 0.10f,
                                        box.getWidth() - shutterInset * 2.0f - notchW * 0.6f,
                                        shutterH * 0.55f);

        // Label area at the bottom
        const float labelInset = box.getWidth() * 0.16f;
        juce::Rectangle<float> label (box.getX() + labelInset,
                                      box.getY() + box.getHeight() * 0.55f,
                                      box.getWidth() - labelInset * 2.0f,
                                      box.getHeight() * 0.32f);

        g.setColour (TEXT_COLOR);
        g.strokePath (body, juce::PathStrokeType (1.4f));
        g.drawRect (shutter, 1.2f);
        g.drawRect (label, 1.2f);
    }

    void drawArrowIcon (juce::Graphics& g, juce::Rectangle<float> region, bool pointLeft) const
    {
        const float cx = region.getCentreX();
        const float cy = region.getCentreY();
        const float hw = region.getWidth() * 0.28f;
        const float ah = region.getHeight() * 0.22f;

        const float tipX  = pointLeft ? cx - hw : cx + hw;
        const float tailX = pointLeft ? cx + hw : cx - hw;

        juce::Path arrow;
        arrow.startNewSubPath (tailX, cy);
        arrow.lineTo (tipX, cy);
        arrow.startNewSubPath (tipX + (pointLeft ?  ah : -ah), cy - ah);
        arrow.lineTo (tipX, cy);
        arrow.lineTo (tipX + (pointLeft ?  ah : -ah), cy + ah);

        g.setColour (TEXT_COLOR);
        g.strokePath (arrow, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void openPresetMenu()
    {
        if (proc == nullptr)
            return;

        std::map<int, juce::File> idToFile;
        auto menu = proc->presetManager.buildMenu (idToFile);
        if (menuLookAndFeel != nullptr)
            menu.setLookAndFeel (menuLookAndFeel);

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
            [this, idToFile = std::move (idToFile)] (int result)
            {
                if (result <= 0)
                    return;
                auto it = idToFile.find (result);
                if (it != idToFile.end() && onLoadFile)
                    onLoadFile (it->second);
            });
    }

    void openSaveDialog()
    {
        if (proc == nullptr)
            return;

        auto* w = new juce::AlertWindow ("Save Preset",
                                         "Enter a name and choose a category.",
                                         juce::MessageBoxIconType::NoIcon);
        w->addTextEditor ("name", proc->currentPresetName, "Name");

        juce::StringArray cats;
        cats.add ("Uncategorized");
        cats.addArray (proc->presetManager.getCategories());
        w->addComboBox ("category", cats, "Category");

        if (auto* combo = w->getComboBoxComponent ("category"))
        {
            combo->setEditableText (true);
            // Pre-select the category of the currently loaded preset, if any.
            const auto curFile = proc->currentPresetFile;
            if (curFile.existsAsFile())
            {
                const auto presetsDir = proc->presetManager.getPresetsDirectory();
                const auto parent = curFile.getParentDirectory();
                if (parent != presetsDir)
                {
                    const int idx = cats.indexOf (parent.getFileName());
                    combo->setSelectedItemIndex (idx >= 0 ? idx : 0);
                }
                else
                {
                    combo->setSelectedItemIndex (0);
                }
            }
            else
            {
                combo->setSelectedItemIndex (0);
            }
        }

        w->addButton ("Save",   1, juce::KeyPress (juce::KeyPress::returnKey));
        w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        if (menuLookAndFeel != nullptr)
            w->setLookAndFeel (menuLookAndFeel);

        w->enterModalState (true,
            juce::ModalCallbackFunction::create ([this, w] (int result)
            {
                std::unique_ptr<juce::AlertWindow> owner (w);
                if (result == 0 || proc == nullptr)
                    return;

                auto name = w->getTextEditorContents ("name").trim();
                if (name.isEmpty())
                {
                    juce::Desktop::getInstance().getDefaultLookAndFeel().playAlertSound();
                    return;
                }

                auto category = w->getComboBoxComponent ("category") != nullptr
                                    ? w->getComboBoxComponent ("category")->getText().trim()
                                    : juce::String();
                if (category == "Uncategorized")
                    category = {};

                auto commitSave = [this, name, category]
                {
                    auto stateTree = proc->buildStateTree();
                    if (! proc->presetManager.savePreset (name, category, stateTree))
                        return;

                    auto savedFile = proc->presetManager.resolvePresetFile (name, category);
                    proc->currentPresetFile = savedFile;
                    proc->currentPresetName = name;
                    setPresetName (name);
                };

                if (proc->presetManager.presetExists (name, category))
                {
                    juce::AlertWindow::showAsync (
                        juce::MessageBoxOptions()
                            .withIconType (juce::MessageBoxIconType::WarningIcon)
                            .withTitle ("Overwrite preset?")
                            .withMessage ("A preset named \"" + name + "\" already exists. Overwrite it?")
                            .withButton ("Overwrite")
                            .withButton ("Cancel"),
                        [commitSave = std::move (commitSave)] (int r)
                        {
                            if (r == 1)
                                commitSave();
                        });
                }
                else
                {
                    commitSave();
                }
            }),
            true);
    }

    PluginProcessor* proc = nullptr;
    juce::LookAndFeel* menuLookAndFeel = nullptr;
    juce::String presetName;

    Region hoveredRegion = Region::None;
    juce::Rectangle<float> saveRegion, nameRegion, prevRegion, nextRegion;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBar)
};
