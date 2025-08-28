#pragma once

// A simple struct to represent a note for UI display purposes.
struct LiveNote {
    int noteNumber;
    int velocity;
    double startTime; // in beats
    double duration;  // in beats
};
