#include "RandomGeneratorV2.h"
#include <random>

RandomGeneratorV2::RandomGeneratorV2()
{
    std::random_device rd;
    randomEngine_.seed(rd());
}

void RandomGeneratorV2::process(juce::MidiBuffer& midiMessages,
                                juce::AudioProcessorValueTreeState& apvts,
                                double sampleRate,
                                double currentBeat)
{
    if (lastBeat_ < 0)
    {
        lastBeat_ = currentBeat;
        nextEventBeat_ = currentBeat;
    }

    while (lastBeat_ < currentBeat)
    {
        if (lastBeat_ >= nextEventBeat_)
        {
            generateEventsAt(nextEventBeat_, midiMessages, apvts, sampleRate);

            auto* baseDurationParam = apvts.getRawParameterValue("RANDOM_V2_BASE_DURATION");
            float baseDurationMap[] = { 16.0f, 8.0f, 4.0f, 3.0f, 2.0f, 1.0f };
            int durationChoice = baseDurationParam ? static_cast<int>(baseDurationParam->load()) : 2;
            double baseDuration = (durationChoice >= 0 && static_cast<size_t>(durationChoice) < std::size(baseDurationMap)) ? baseDurationMap[durationChoice] : 4.0;
            nextEventBeat_ += baseDuration;
        }
        lastBeat_ += 0.001; // Small increment to avoid infinite loops, real timing is handled by nextEventBeat_
    }
}

void RandomGeneratorV2::generateEventsAt(double beat, juce::MidiBuffer& midiMessages, juce::AudioProcessorValueTreeState& apvts, double sampleRate)
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
                addNote(midiMessages, apvts, sampleRate, beat + (i * noteInterval));
            }
        }
    }
    else if (dist(randomEngine_) < noteProbParam->load())
    {
        addNote(midiMessages, apvts, sampleRate, beat);
    }
}


void RandomGeneratorV2::addNote(juce::MidiBuffer& midiMessages, juce::AudioProcessorValueTreeState& apvts, double sampleRate, double beat)
{
    auto* minNoteParam = apvts.getRawParameterValue("RANDOM_V2_MIN_NOTE");
    auto* maxNoteParam = apvts.getRawParameterValue("RANDOM_V2_MAX_NOTE");
    auto* channelParam = apvts.getRawParameterValue("MIDI_CHANNEL");
    auto* bpmParam = apvts.getRawParameterValue("BPM");

    if (!minNoteParam || !maxNoteParam || !channelParam || !bpmParam) return;

    int minNote = static_cast<int>(minNoteParam->load());
    int maxNote = static_cast<int>(maxNoteParam->load());
    int channel = static_cast<int>(channelParam->load());
    double bpm = bpmParam->load();

    int noteNumber = static_cast<int>(randomEngine_() % (maxNote - minNote + 1)) + minNote;
    int velocity = static_cast<int>(randomEngine_() % 60) + 40; // 40-99

    double durationInBeats = 1.0; // Fixed duration for now
    int durationInSamples = static_cast<int>(durationInBeats * (60.0 / bpm) * sampleRate);

    int samplePos = static_cast<int>(beat * (60.0 / bpm) * sampleRate);

    midiMessages.addEvent(juce::MidiMessage::noteOn(channel, noteNumber, (juce::uint8)velocity), samplePos);
    midiMessages.addEvent(juce::MidiMessage::noteOff(channel, noteNumber, samplePos + durationInSamples));
}

void RandomGeneratorV2::setScale(int rootNote, const std::vector<int>& scaleNotes)
{
    juce::ignoreUnused(rootNote, scaleNotes);
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
        generateEventsAt(currentBeat, pattern, apvts, sampleRate);
        currentBeat += baseDuration;
    }

    return pattern;
}
