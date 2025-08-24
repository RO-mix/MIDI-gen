#pragma once
#include <JuceHeader.h>

class Scales
{
public:
    static const std::map<juce::String, std::vector<int>> scaleIntervals;

    static bool isNoteInScale(int note, const juce::String& scaleName);
    static std::vector<int> getScaleNotes(int rootNote, const juce::String& scaleName);
    static std::vector<int> getScaleNotesInRange(int rootNote, const juce::String& scaleName, int minNote = 0, int maxNote = 127);

    static juce::StringArray getAvailableScaleNames();

private:
    // Основные лады
    static std::vector<int> major;
    static std::vector<int> minor;
    static std::vector<int> dorian;
    static std::vector<int> phrygian;
    static std::vector<int> lydian;
    static std::vector<int> mixolydian;
    static std::vector<int> locrian;
    static std::vector<int> harmonicMinor;
    static std::vector<int> melodicMinor;
    static std::vector<int> majorPentatonic;
    static std::vector<int> minorPentatonic;
    static std::vector<int> blues;
    static std::vector<int> wholeTone;
    static std::vector<int> chromatic;
    // Дополнительные экзотические лады
    static std::vector<int> hirajoshi;
    static std::vector<int> iwato;
    static std::vector<int> hungarianMinor;
    static std::vector<int> phrygianDominant;
    static std::vector<int> enigmatic;
};