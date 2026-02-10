#pragma once

struct TempoInfo
{
    double bpm = 120.0;
    double ppqPosition = 0.0;
    bool isPlaying = false;
    bool hostTempoAvailable = false;
};
