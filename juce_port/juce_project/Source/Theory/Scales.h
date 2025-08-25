#pragma once
#include <JuceHeader.h>

enum class ScaleType
{
    Major = 0,
    Minor = 1,
    Dorian = 2,
    Phrygian = 3,
    Lydian = 4,
    Mixolydian = 5,
    Locrian = 6,
    HarmonicMinor = 7,
    MelodicMinor = 8,
    MajorPentatonic = 9,
    MinorPentatonic = 10,
    Blues = 11,
    WholeTone = 12,
    Chromatic = 13,
    Hirajoshi = 14,
    Iwato = 15,
    HungarianMinor = 16,
    PhrygianDominant = 17,
    Enigmatic = 18
};

class Scales
{
public:
    static const std::map<juce::String, std::vector<int>> scaleIntervals;
    static const std::map<ScaleType, juce::String> scaleTypeToName;

    static bool isNoteInScale(int note, const juce::String& scaleName);
    static std::vector<int> getScaleNotes(int rootNote, const juce::String& scaleName);
    static std::vector<int> getScaleNotesInRange(int rootNote, const juce::String& scaleName, int minNote = 0, int maxNote = 127);

    static juce::StringArray getAvailableScaleNames();
    static juce::String getScaleName(ScaleType type);
    static ScaleType getScaleTypeFromInt(int type);
    
    // Получить ноты аккорда для заданного лада, корневой ноты и степени аккорда (0 = I, 1 = ii, ...)
    static std::vector<int> getChordNotes(int rootNote, ScaleType scaleType, int chordDegree);

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