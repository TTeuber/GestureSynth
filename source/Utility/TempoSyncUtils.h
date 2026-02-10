#pragma once

namespace TempoSync
{

// Duration of each note division in quarter-note beats.
// Index order: 4/1, 2/1, 1/1, 1/2, 1/2T, 1/2D, 1/4, 1/4T, 1/4D, 1/8, 1/8T, 1/8D, 1/16, 1/16T, 1/16D, 1/32
inline constexpr double beatsPerCycle[] = {
    16.0,           // 4/1
    8.0,            // 2/1
    4.0,            // 1/1
    2.0,            // 1/2
    4.0 / 3.0,      // 1/2T  (triplet = base * 2/3)
    3.0,            // 1/2D  (dotted = base * 3/2)
    1.0,            // 1/4
    2.0 / 3.0,      // 1/4T
    1.5,            // 1/4D
    0.5,            // 1/8
    1.0 / 3.0,      // 1/8T
    0.75,           // 1/8D
    0.25,           // 1/16
    1.0 / 6.0,      // 1/16T
    0.375,          // 1/16D
    0.125           // 1/32
};

inline constexpr int numDivisions = 16;

/** Convert a note division index + BPM to an LFO rate in Hz. */
inline double noteDivisionToHz (double bpm, int divisionIndex)
{
    if (divisionIndex < 0 || divisionIndex >= numDivisions)
        divisionIndex = 6; // default to 1/4
    return (bpm / 60.0) / beatsPerCycle[divisionIndex];
}

} // namespace TempoSync
