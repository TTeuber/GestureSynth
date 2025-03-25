//
// Created by Tyler Teuber on 3/19/25.
//

#include "MyOscillator.h"

void MyOscillator::setWaveform (Waveform newWaveform)
{
    switch (newWaveform)
    {
        case Sine:
            initialise ([this] (float x) {
                return sineWave (x);
            },
                128);
            break;
        case Saw:
            initialise ([this] (float x) {
                return sawWave (x);
            },
                2);
            break;
        case Square:
            initialise ([this] (float x) {
                return squareWave (x);
            },
                0);
            break;
        case Triangle:
            initialise ([this] (float x) {
                return triangleWave (x);
            },
                5);
            break;
    }
}