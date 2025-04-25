//
// Created by Tyler Teuber on 3/19/25.
//

#include "MyOscillator.h"

void MyOscillator::setWaveform (const Waveform newWaveform)
{
    switch (newWaveform)
    {
        case Sine:
            initialise ([this] (const float x) {
                return sineWave (x);
            },
                128);
            break;
        case Saw:
            initialise ([this] (const float x) {
                return sawWave (x);
            },
                0);
            break;
        case Square:
            initialise ([this] (const float x) {
                return squareWave (x);
            },
                0);
            break;
        case Triangle:
            initialise ([this] (const float x) {
                return triangleWave (x);
            },
                5);
            break;
    }
}