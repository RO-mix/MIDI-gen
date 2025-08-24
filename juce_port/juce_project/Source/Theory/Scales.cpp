#include "Scales.h"

// Инициализация статических членов
std::vector<int> Scales::major = {0, 2, 4, 5, 7, 9, 11};
std::vector<int> Scales::minor = {0, 2, 3, 5, 7, 8, 10};
std::vector<int> Scales::dorian = {0, 2, 3, 5, 7, 9, 10};
std::vector<int> Scales::phrygian = {0, 1, 3, 5, 7, 8, 10};
std::vector<int> Scales::lydian = {0, 2, 4, 6, 7, 9, 11};
std::vector<int> Scales::mixolydian = {0, 2, 4, 5, 7, 9, 10};
std::vector<int> Scales::locrian = {0, 1, 3, 4, 7, 8, 10};
std::vector<int> Scales::harmonicMinor = {0, 2, 3, 5, 7, 8, 11};
std::vector<int> Scales::melodicMinor = {0, 2, 3, 5, 7, 9, 11};
std::vector<int> Scales::majorPentatonic = {0, 2, 4, 7, 9};
std::vector<int> Scales::minorPentatonic = {0, 3, 5, 7, 10};
std::vector<int> Scales::blues = {0, 3, 5, 6, 7, 10};
std::vector<int> Scales::wholeTone = {0, 2, 4, 6, 8, 10};
std::vector<int> Scales::chromatic = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
std::vector<int> Scales::hirajoshi = {0, 2, 3, 7, 8};
std::vector<int> Scales::iwato = {0, 1, 5, 6, 10};
std::vector<int> Scales::hungarianMinor = {0, 2, 3, 6, 7, 8, 11};
std::vector<int> Scales::phrygianDominant = {0, 1, 4, 5, 7, 8, 10};
std::vector<int> Scales::enigmatic = {0, 1, 4, 6, 8, 10, 11};

// Карта соответствия имен ладов и их интервалов
const std::map<juce::String, std::vector<int>> Scales::scaleIntervals = {
    {"Major", major},
    {"Minor", minor},
    {"Dorian", dorian},
    {"Phrygian", phrygian},
    {"Lydian", lydian},
    {"Mixolydian", mixolydian},
    {"Locrian", locrian},
    {"Harmonic Minor", harmonicMinor},
    {"Melodic Minor", melodicMinor},
    {"Major Pentatonic", majorPentatonic},
    {"Minor Pentatonic", minorPentatonic},
    {"Blues", blues},
    {"Whole Tone", wholeTone},
    {"Chromatic", chromatic},
    {"Hirajoshi", hirajoshi},
    {"Iwato", iwato},
    {"Hungarian Minor", hungarianMinor},
    {"Phrygian Dominant", phrygianDominant},
    {"Enigmatic", enigmatic}
};

bool Scales::isNoteInScale(int note, const juce::String& scaleName)
{
    auto it = scaleIntervals.find(scaleName);
    if (it == scaleIntervals.end())
        return false;

    // Получаем ноту относительно октавы (0-11)
    int noteInOctave = note % 12;

    // Проверяем, есть ли нота в интервалах лада
    const auto& intervals = it->second;
    return std::find(intervals.begin(), intervals.end(), noteInOctave) != intervals.end();
}

std::vector<int> Scales::getScaleNotes(int rootNote, const juce::String& scaleName)
{
    return getScaleNotesInRange(rootNote, scaleName, 0, 127);
}

std::vector<int> Scales::getScaleNotesInRange(int rootNote, const juce::String& scaleName, int minNote, int maxNote)
{
    auto it = scaleIntervals.find(scaleName);
    if (it == scaleIntervals.end())
        return {};

    const auto& intervals = it->second;
    if (intervals.empty())
        return {};

    std::vector<int> notes;

    // Генерируем ноты для всех октав
    for (int octave = -1; octave <= 10; ++octave) {  // От C-1 до C10
        for (int interval : intervals) {
            int note = rootNote + interval + (octave * 12);
            if (note >= minNote && note <= maxNote) {
                notes.push_back(note);
            }
        }
    }

    // Сортируем и удаляем дубликаты
    std::sort(notes.begin(), notes.end());
    auto last = std::unique(notes.begin(), notes.end());
    notes.erase(last, notes.end());

    return notes;
}

juce::StringArray Scales::getAvailableScaleNames()
{
    juce::StringArray names;
    for (const auto& pair : scaleIntervals) {
        names.add(pair.first);
    }
    return names;
}