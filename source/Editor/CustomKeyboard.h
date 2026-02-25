#pragma once

#include "../Theme.h"
#include <juce_audio_utils/juce_audio_utils.h>

class CustomKeyboard final : public juce::Component, private juce::Timer
{
public:
    explicit CustomKeyboard (juce::MidiKeyboardState& state)
        : keyboardState (state)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setWantsKeyboardFocus (true);
        startTimerHz (30);
    }

    ~CustomKeyboard() override
    {
        stopTimer();
        // Release any computer-keyboard-held notes
        for (int i = 0; i < 128; ++i)
            if (keysPressed[i])
                keyboardState.noteOff (midiChannel, i, 0.0f);

        if (currentNote >= 0)
        {
            keyboardState.noteOff (midiChannel, currentNote, 0.0f);
            currentNote = -1;
        }
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (30, 35, 35));

        const auto bounds = getLocalBounds().toFloat();
        const float whiteKeyWidth = bounds.getWidth() / static_cast<float> (numWhiteKeys);
        const float whiteKeyHeight = bounds.getHeight();
        const float blackKeyWidth = whiteKeyWidth * 0.6f;
        const float blackKeyHeight = whiteKeyHeight * 0.62f;

        // Pass 1: white keys
        int whiteIndex = 0;
        for (int note = lowestNote; note <= highestNote; ++note)
        {
            if (isBlackKey (note))
                continue;

            const float x = static_cast<float> (whiteIndex) * whiteKeyWidth;
            auto keyRect = juce::Rectangle<float> (x, 0.0f, whiteKeyWidth, whiteKeyHeight);

            bool pressed = keyboardState.isNoteOn (midiChannel, note);
            bool hovered = (note == hoveredNote && !pressed);

            if (pressed)
                g.setColour (juce::Colour (140, 150, 150));
            else if (hovered)
                g.setColour (juce::Colour (175, 180, 178));
            else
                g.setColour (juce::Colour (200, 200, 195));

            g.fillRect (keyRect.reduced (0.5f, 0.0f));

            // Border
            g.setColour (juce::Colour (50, 55, 55));
            g.drawRect (keyRect, 1.0f);

            // C labels
            if (note % 12 == 0)
            {
                int octave = (note / 12) - 2; // MIDI 24 = C0
                g.setColour (juce::Colour (90, 95, 95));
                g.setFont (juce::jmin (11.0f, whiteKeyWidth * 0.55f));
                auto labelArea = keyRect.removeFromBottom (18.0f);
                g.drawText ("C" + juce::String (octave), labelArea, juce::Justification::centred, false);
            }

            ++whiteIndex;
        }

        // Pass 2: black keys
        whiteIndex = 0;
        for (int note = lowestNote; note <= highestNote; ++note)
        {
            if (isBlackKey (note))
            {
                // Black key sits to the left of the next white key
                const float x = static_cast<float> (whiteIndex) * whiteKeyWidth - blackKeyWidth * 0.5f;
                auto keyRect = juce::Rectangle<float> (x, 0.0f, blackKeyWidth, blackKeyHeight);

                bool pressed = keyboardState.isNoteOn (midiChannel, note);
                bool hovered = (note == hoveredNote && !pressed);

                if (pressed)
                    g.setColour (juce::Colour (60, 75, 75));
                else if (hovered)
                    g.setColour (juce::Colour (45, 50, 50));
                else
                    g.setColour (juce::Colour (25, 30, 30));

                g.fillRoundedRectangle (keyRect, 2.0f);

                g.setColour (juce::Colour (50, 55, 55));
                g.drawRoundedRectangle (keyRect, 2.0f, 0.5f);
            }
            else
            {
                ++whiteIndex;
            }
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        int note = getNoteAtPosition (e.position);
        if (note >= 0)
        {
            currentNote = note;
            keyboardState.noteOn (midiChannel, note, 0.8f);
            repaint();
        }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        int note = getNoteAtPosition (e.position);
        if (note != currentNote)
        {
            if (currentNote >= 0)
                keyboardState.noteOff (midiChannel, currentNote, 0.0f);
            currentNote = note;
            if (note >= 0)
                keyboardState.noteOn (midiChannel, note, 0.8f);
            repaint();
        }
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (currentNote >= 0)
        {
            keyboardState.noteOff (midiChannel, currentNote, 0.0f);
            currentNote = -1;
            repaint();
        }
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        int note = getNoteAtPosition (e.position);
        if (note != hoveredNote)
        {
            hoveredNote = note;
            repaint();
        }
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        if (hoveredNote >= 0)
        {
            hoveredNote = -1;
            repaint();
        }
    }

    bool keyPressed (const juce::KeyPress& key) override
    {
        return keyMapping.indexOfChar (key.getTextCharacter()) >= 0;
    }

    bool keyStateChanged (bool) override
    {
        bool changed = false;

        for (int i = 0; i < keyMapping.length(); ++i)
        {
            const int note = 12 * keyMappingOctave + i;
            if (note < 0 || note > 127)
                continue;

            const bool isDown = juce::KeyPress::isKeyCurrentlyDown (keyMapping[i]);

            if (isDown != keysPressed[note])
            {
                keysPressed.setBit (note, isDown);

                if (isDown)
                    keyboardState.noteOn (midiChannel, note, 0.8f);
                else
                    keyboardState.noteOff (midiChannel, note, 0.0f);

                changed = true;
            }
        }

        return changed;
    }

    void focusLost (FocusChangeType) override
    {
        for (int i = 0; i < 128; ++i)
        {
            if (keysPressed[i])
            {
                keyboardState.noteOff (midiChannel, i, 0.0f);
                keysPressed.clearBit (i);
            }
        }
    }

private:
    void timerCallback() override
    {
        repaint();
    }

    static bool isBlackKey (int midiNote)
    {
        const int n = midiNote % 12;
        return n == 1 || n == 3 || n == 6 || n == 8 || n == 10;
    }

    int getNoteAtPosition (juce::Point<float> pos) const
    {
        const auto bounds = getLocalBounds().toFloat();
        if (!bounds.contains (pos))
            return -1;

        const float whiteKeyWidth = bounds.getWidth() / static_cast<float> (numWhiteKeys);
        const float blackKeyWidth = whiteKeyWidth * 0.6f;
        const float blackKeyHeight = bounds.getHeight() * 0.62f;

        // Check black keys first (they overlap white keys)
        if (pos.y < blackKeyHeight)
        {
            int whiteIndex = 0;
            for (int note = lowestNote; note <= highestNote; ++note)
            {
                if (isBlackKey (note))
                {
                    const float x = static_cast<float> (whiteIndex) * whiteKeyWidth - blackKeyWidth * 0.5f;
                    auto keyRect = juce::Rectangle<float> (x, 0.0f, blackKeyWidth, blackKeyHeight);
                    if (keyRect.contains (pos))
                        return note;
                }
                else
                {
                    ++whiteIndex;
                }
            }
        }

        // Check white keys
        int whiteIndex = 0;
        for (int note = lowestNote; note <= highestNote; ++note)
        {
            if (isBlackKey (note))
                continue;

            const float x = static_cast<float> (whiteIndex) * whiteKeyWidth;
            auto keyRect = juce::Rectangle<float> (x, 0.0f, whiteKeyWidth, bounds.getHeight());
            if (keyRect.contains (pos))
                return note;

            ++whiteIndex;
        }

        return -1;
    }

    juce::MidiKeyboardState& keyboardState;

    static constexpr int lowestNote = 24;   // C0
    static constexpr int highestNote = 83;  // B5
    static constexpr int numWhiteKeys = 42; // C0-B5: 42 white keys
    static constexpr int midiChannel = 1;

    int currentNote = -1;
    int hoveredNote = -1;

    // Computer keyboard input
    juce::BigInteger keysPressed;
    int keyMappingOctave = 5;
    const juce::String keyMapping { "awsedftgyhujkolp;" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomKeyboard)
};
