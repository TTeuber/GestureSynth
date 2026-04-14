//
// Created by Tyler Teuber 6/1/25
//

#pragma once

#include <utility>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../../Theme.h"

namespace InlineParameterEditUtils
{
inline juce::String extractEditableText (const juce::String& displayText)
{
    auto trimmed = displayText.trim();
    auto colonIndex = trimmed.lastIndexOfChar (':');

    if (colonIndex >= 0)
    {
        auto afterColon = trimmed.substring (colonIndex + 1).trim();
        if (afterColon.isNotEmpty())
            return afterColon;
    }

    return trimmed;
}

inline std::pair<juce::String, juce::String> splitNumericAndSuffix (const juce::String& text)
{
    auto trimmed = text.trim();
    int index = 0;

    while (index < trimmed.length())
    {
        auto character = trimmed[index];
        if (! juce::CharacterFunctions::isDigit (character)
            && character != '.'
            && character != '-'
            && character != '+')
        {
            break;
        }

        ++index;
    }

    return { trimmed.substring (0, index).trim(), trimmed.substring (index).trim() };
}

inline float parseNormalizedValue (juce::RangedAudioParameter* param,
    const juce::String& enteredText,
    const juce::String& displayText)
{
    jassert (param != nullptr);

    auto valueText = extractEditableText (enteredText);
    auto referenceText = extractEditableText (displayText);
    auto [numericText, suffix] = splitNumericAndSuffix (valueText);
    auto [referenceNumericText, referenceSuffix] = splitNumericAndSuffix (referenceText);

    juce::ignoreUnused (referenceNumericText);

    if (numericText.isEmpty())
        return juce::jlimit (0.0f, 1.0f, param->getValueForText (valueText));

    auto numericValue = numericText.getFloatValue();
    auto normalizedRange = param->getNormalisableRange();
    auto lowerSuffix = suffix.toLowerCase().removeCharacters (" ");
    auto lowerReferenceSuffix = referenceSuffix.toLowerCase().removeCharacters (" ");

    // When the user omits the suffix, inherit it from the reference display —
    // a bare number typed into a field showing "ms" should be interpreted as ms.
    auto effectiveSuffix = lowerSuffix.isEmpty() ? lowerReferenceSuffix : lowerSuffix;

    if (effectiveSuffix.containsChar ('%'))
        return juce::jlimit (0.0f, 1.0f, numericValue / 100.0f);

    if (effectiveSuffix == "cents" || effectiveSuffix == "cent")
        numericValue /= 100.0f;
    else if (effectiveSuffix == "khz")
        numericValue *= 1000.0f;
    else if (effectiveSuffix == "ms" && normalizedRange.end <= 1.0f)
        numericValue /= 1000.0f;

    return juce::jlimit (0.0f, 1.0f, param->convertTo0to1 (numericValue));
}
}

class InlineParameterEditor : private juce::MouseListener
{
public:
    using CommitCallback = std::function<void (const juce::String&)>;
    using CancelCallback = std::function<void()>;

    explicit InlineParameterEditor (juce::Component& owner)
        : owner (owner)
    {
        editor.setColour (juce::TextEditor::backgroundColourId, SECONDARY_COLOR);
        editor.setColour (juce::TextEditor::textColourId, TEXT_COLOR);
        editor.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        editor.setColour (juce::CaretComponent::caretColourId, TEXT_COLOR);
        editor.setJustification (juce::Justification::centred);
        editor.setSelectAllWhenFocused (true);
        editor.setScrollbarsShown (false);
        editor.onReturnKey = [this] { commit(); };
        editor.onEscapeKey = [this] { cancel(); };
        editor.onFocusLost = [this] { commitOnFocusLost(); };
        editor.onTextChange = [this] { textChanged = true; };
        owner.addChildComponent (editor);
        editor.setVisible (false);
    }

    ~InlineParameterEditor() override
    {
        removeGlobalMouseListener();
    }

    void beginEdit (juce::Rectangle<int> bounds,
        juce::String initialValue,
        CommitCallback onCommit,
        CancelCallback onCancel = {})
    {
        commitCallback = std::move (onCommit);
        cancelCallback = std::move (onCancel);
        initialText = std::move (initialValue);
        textChanged = false;
        editor.setBounds (bounds);
        editor.setText (initialText, juce::dontSendNotification);
        editor.setVisible (true);
        editor.toFront (false);
        editor.grabKeyboardFocus();
        editor.selectAll();
        addGlobalMouseListener();
    }

    void cancel()
    {
        if (! isEditing())
            return;

        hidingEditor = true;
        editor.setVisible (false);
        editor.giveAwayKeyboardFocus();
        hidingEditor = false;

        if (cancelCallback != nullptr)
            cancelCallback();

        clearCallbacks();
    }

    bool isEditing() const
    {
        return editor.isVisible();
    }

private:
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! isEditing() || hidingEditor)
            return;

        auto* originalComponent = e.originalComponent;
        if (originalComponent == &editor || editor.isParentOf (originalComponent))
            return;

        // Owner's own mouseDown fires before this global listener (see
        // Component::internalMouseDown), and its isEditing() check already
        // swallows the click — so we just commit/cancel here and do not need
        // to flag any follow-up mouseDown for consumption.
        if (textChanged && editor.getText() != initialText)
            commit();
        else
            cancel();
    }

    void commit()
    {
        if (! isEditing())
            return;

        auto text = editor.getText().trim();

        hidingEditor = true;
        editor.setVisible (false);
        editor.giveAwayKeyboardFocus();
        hidingEditor = false;

        if (commitCallback != nullptr)
            commitCallback (text);

        clearCallbacks();
    }

    void commitOnFocusLost()
    {
        if (! isEditing() || hidingEditor)
            return;

        if (textChanged && editor.getText() != initialText)
            commit();
        else
            cancel();
    }

    void clearCallbacks()
    {
        removeGlobalMouseListener();
        commitCallback = {};
        cancelCallback = {};
        initialText.clear();
        textChanged = false;
    }

    void addGlobalMouseListener()
    {
        if (globalMouseListenerRegistered)
            return;

        juce::Desktop::getInstance().addGlobalMouseListener (this);
        globalMouseListenerRegistered = true;
    }

    void removeGlobalMouseListener()
    {
        if (! globalMouseListenerRegistered)
            return;

        juce::Desktop::getInstance().removeGlobalMouseListener (this);
        globalMouseListenerRegistered = false;
    }

    juce::Component& owner;
    juce::TextEditor editor;
    CommitCallback commitCallback;
    CancelCallback cancelCallback;
    juce::String initialText;
    bool textChanged = false;
    bool hidingEditor = false;
    bool globalMouseListenerRegistered = false;
};
