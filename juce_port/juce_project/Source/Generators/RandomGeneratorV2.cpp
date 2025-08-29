#include "RandomGeneratorV2.h"
#include <random>

RandomGeneratorV2::RandomGeneratorV2()
{
    std::random_device rd;
    randomEngine_.seed(rd());
}

juce::Array<PendingNoteOff> RandomGeneratorV2::process(juce::MidiBuffer& midiMessages,
                                                       juce::AudioProcessorValueTreeState& apvts,
                                                       double sampleRate,
                                                       double blockStartTime,
                                                       double blockEndTime,
                                                       int numSamples)
{
    juce::ignoreUnused(numSamples);

    if (lastBeat_ < 0)
    {
        lastBeat_ = blockStartTime;
        nextEventBeat_ = blockStartTime;
    }

    // This logic needs to be adapted for block processing.
    // We check if the next event falls within the current block.
    while (nextEventBeat_ < blockEndTime)
    {
        if (nextEventBeat_ >= blockStartTime)
        {
            // This event should be generated in this block.
            generateEventsAt(nextEventBeat_, midiMessages, apvts, sampleRate, blockStartTime);
        }

        auto* baseDurationParam = apvts.getRawParameterValue("RANDOM_V2_BASE_DURATION");
        float baseDurationMap[] = { 16.0f, 8.0f, 4.0f, 3.0f, 2.0f, 1.0f };
        int durationChoice = baseDurationParam ? static_cast<int>(baseDurationParam->load()) : 2;
        double baseDuration = (durationChoice >= 0 && static_cast<size_t>(durationChoice) < std::size(baseDurationMap)) ? baseDurationMap[durationChoice] : 4.0;
        nextEventBeat_ += baseDuration;
    }

    // lastBeat_ is now correctly preserved across blocks.
    return {};
}

void RandomGeneratorV2::generateEventsAt(double beat, juce::MidiBuffer& midiMessages, juce::AudioProcessorValueTreeState& apvts, double sampleRate, double blockStartTime)
{
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    auto* burstProbParam = apvts.getRawParameterValue("RANDOM_V2_BURST_PROB");
    auto* noteProbParam = apvts.getRawParameterValue("RANDOM_V2_NOTE_PROB");

    if (!burstProbParam || !noteProbParam) return;

    if (dist(randomEngine_) < burstProbParam->load())
    {
        auto* accelParam = apvts.getRawParameterValue("RANDOM_V2_ACCELERATION");
        if (!accelParam) return;

        float accelMap[] = { 1.0f, 0.5f, 0.25f, 0.125f };
        int accelChoice = static_cast<int>(accelParam->load());
        double noteInterval = (accelChoice >= 0 && static_cast<size_t>(accelChoice) < std::size(accelMap)) ? accelMap[accelChoice] : 0.25;

        for (int i = 0; i < 8; ++i)
        {
            auto* patternParam = apvts.getRawParameterValue("RANDOM_V2_BURST_PATTERN_" + juce::String(i));
            if (patternParam && dist(randomEngine_) < patternParam->load())
            {
                addNote(midiMessages, apvts, sampleRate, beat + (i * noteInterval), blockStartTime);
            }
        }
    }
    else if (dist(randomEngine_) < noteProbParam->load())
    {
        addNote(midiMessages, apvts, sampleRate, beat, blockStartTime);
    }
}


void RandomGeneratorV2::addNote(juce::MidiBuffer& midiMessages, juce::AudioProcessorValueTreeState& apvts, double sampleRate, double beat, double blockStartTime)
{
    auto* minNoteParam = apvts.getRawParameterValue("RANDOM_V2_MIN_NOTE");
    auto* maxNoteParam = apvts.getRawParameterValue("RANDOM_V2_MAX_NOTE");
    auto* channelParam = apvts.getRawParameterValue("MIDI_CHANNEL");
    auto* bpmParam = apvts.getRawParameterValue("BPM");

    if (!minNoteParam || !maxNoteParam || !channelParam || !bpmParam || scaleNotes_.empty()) return;

    int minNote = static_cast<int>(minNoteParam->load());
    int maxNote = static_cast<int>(maxNoteParam->load());
    int channel = static_cast<int>(channelParam->load());
    double bpm = bpmParam->load();

    // 1. Get all scale notes within the min/max range
    std::vector<int> notesInRange;
    for (int octave = 0; octave < 10; ++octave)
    {
        for (int scaleDegree : scaleNotes_)
        {
            int note = rootNote_ + (octave * 12) + scaleDegree;
            if (note >= minNote && note <= maxNote)
            {
                notesInRange.push_back(note);
            }
        }
    }

    if (notesInRange.empty()) return;

    int noteNumber;

    // 2. Bass note filtering logic
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    if (dist(randomEngine_) < 0.5f)
    {
        // 50% chance to play a bass note (root or fifth)
        std::vector<int> bassNotes;
        int root = rootNote_ % 12;
        int fifth = (root + 7) % 12;

        for (int note : notesInRange)
        {
            if (note % 12 == root || note % 12 == fifth)
            {
                bassNotes.push_back(note);
            }
        }

        if (!bassNotes.empty())
        {
            // Find the lowest octave for bass notes
            int lowestOctave = 10;
            for (int note : bassNotes) {
                lowestOctave = std::min(lowestOctave, note / 12);
            }
            std::vector<int> lowestBassNotes;
            for (int note : bassNotes) {
                if (note / 12 == lowestOctave) {
                    lowestBassNotes.push_back(note);
                }
            }
            if(!lowestBassNotes.empty())
                noteNumber = lowestBassNotes[randomEngine_() % lowestBassNotes.size()];
            else
                noteNumber = notesInRange[randomEngine_() % notesInRange.size()];
        }
        else
        {
            // No bass notes in range, so just pick a random note
            noteNumber = notesInRange[randomEngine_() % notesInRange.size()];
        }
    }
    else
    {
        // 50% chance to play any note from the scale
        noteNumber = notesInRange[randomEngine_() % notesInRange.size()];
    }

    int velocity = static_cast<int>(randomEngine_() % 60) + 40; // 40-99
    double durationInBeats = 1.0; // Fixed duration for now
    int durationInSamples = static_cast<int>(durationInBeats * (60.0 / bpm) * sampleRate);
    double beatsToSamples = (60.0 / bpm) * sampleRate;
    int samplePos = static_cast<int>((beat - blockStartTime) * beatsToSamples);


    midiMessages.addEvent(juce::MidiMessage::noteOn(channel, noteNumber, (juce::uint8)velocity), samplePos);
    auto noteOffMessage = juce::MidiMessage::noteOff(channel, noteNumber);
    midiMessages.addEvent(noteOffMessage, samplePos + durationInSamples);
}

void RandomGeneratorV2::setScale(int rootNote, const std::vector<int>& scaleNotes)
{
    rootNote_ = rootNote;
    scaleNotes_ = scaleNotes;
}

juce::MidiBuffer RandomGeneratorV2::getPattern(double durationInBeats, juce::AudioProcessorValueTreeState& apvts, double sampleRate)
{
    juce::MidiBuffer pattern;
    double currentBeat = 0.0;

    auto* baseDurationParam = apvts.getRawParameterValue("RANDOM_V2_BASE_DURATION");
    float baseDurationMap[] = { 16.0f, 8.0f, 4.0f, 3.0f, 2.0f, 1.0f };
    int durationChoice = baseDurationParam ? static_cast<int>(baseDurationParam->load()) : 2;
    double baseDuration = (durationChoice >= 0 && static_cast<size_t>(durationChoice) < std::size(baseDurationMap)) ? baseDurationMap[durationChoice] : 4.0;

    if (baseDuration <= 0) baseDuration = 4.0;

    while (currentBeat < durationInBeats)
    {
        generateEventsAt(currentBeat, pattern, apvts, sampleRate, currentBeat);
        currentBeat += baseDuration;
    }

    return pattern;
}

void RandomGeneratorV2::reset()
{
    lastBeat_ = -1.0;
    nextEventBeat_ = 0.0;
}
