#pragma once
#include <JuceHeader.h>

class Looper
{
public:
    void recordNote(const juce::MidiMessage& message, double beatTime);
    void startPlayback();
    void stopPlayback();
    void clear();
    
    juce::MidiBuffer getPlaybackBuffer(int numSamples, double startTime, double endTime);
    
private:
    struct RecordedNote {
        juce::MidiMessage message;
        double beatTime;
    };
    
    std::vector<RecordedNote> recordedNotes;
    bool isRecording = false;
    bool isPlaying = false;
    double loopStart = 0.0;
    double loopEnd = 4.0; // 4 beats
};