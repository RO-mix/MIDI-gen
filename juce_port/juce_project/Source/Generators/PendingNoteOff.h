#pragma once
#include <JuceHeader.h>

struct PendingNoteOff
{
    int noteNumber;
    int channel;
    juce::int64 sampleOffTime;
};
