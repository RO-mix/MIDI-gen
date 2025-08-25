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
    // This is a placeholder implementation and can be greatly improved.
    // It demonstrates using the parameters to create the "ambient burst" effect.

    if (lastBeat_ < 0) lastBeat_ = currentBeat;

    auto* burstProbParam = apvts.getRawParameterValue("RANDOM_V2_BURST_PROB");
    auto* noteProbParam = apvts.getRawParameterValue("RANDOM_V2_NOTE_PROB");
    auto* baseDurationParam = apvts.getRawParameterValue("RANDOM_V2_BASE_DURATION");

    if (!burstProbParam || !noteProbParam || !baseDurationParam) return;

    float baseDurationMap[] = { 16.0f, 8.0f, 4.0f, 3.0f, 2.0f, 1.0f };
    int durationChoice = static_cast<int>(*baseDurationParam);
    double baseDuration = (durationChoice >= 0 && durationChoice < std::size(baseDurationMap)) ? baseDurationMap[durationChoice] : 4.0;

    // Check if it's time for a new event
    if (currentBeat >= nextEventBeat_)
    {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        // Decide whether to burst or play a single note
        if (dist(randomEngine_) < *burstProbParam)
        {
            // --- Burst ---
            auto* accelParam = apvts.getRawParameterValue("RANDOM_V2_ACCELERATION");
            if (!accelParam) return;

            float accelMap[] = { 1.0f, 0.5f, 0.25f, 0.125f }; // Simplified mapping
            int accelChoice = static_cast<int>(*accelParam);
            double noteInterval = (accelChoice >= 0 && accelChoice < std::size(accelMap)) ? accelMap[accelChoice] : 0.25;

            for (int i = 0; i < 8; ++i)
            {
                auto* patternParam = apvts.getRawParameterValue("RANDOM_V2_BURST_PATTERN_" + juce::String(i));
                if (patternParam && dist(randomEngine_) < *patternParam)
                {
                    addNote(midiMessages, apvts, sampleRate, nextEventBeat_ + (i * noteInterval));
                }
            }
        }
        else if (dist(randomEngine_) < *noteProbParam)
        {
            // --- Single Note ---
            addNote(midiMessages, apvts, sampleRate, nextEventBeat_);
        }

        nextEventBeat_ += baseDuration;
    }

    lastBeat_ = currentBeat;
}

void RandomGeneratorV2::addNote(juce::MidiBuffer& midiMessages, juce::AudioProcessorValueTreeState& apvts, double sampleRate, double beat)
{
    auto* minNoteParam = apvts.getRawParameterValue("RANDOM_V2_MIN_NOTE");
    auto* maxNoteParam = apvts.getRawParameterValue("RANDOM_V2_MAX_NOTE");
    auto* channelParam = apvts.getRawParameterValue("MIDI_CHANNEL");
    auto* bpmParam = apvts.getRawParameterValue("BPM");

    if (!minNoteParam || !maxNoteParam || !channelParam || !bpmParam) return;

    int minNote = *minNoteParam;
    int maxNote = *maxNoteParam;
    int channel = *channelParam;
    double bpm = *bpmParam;

    int noteNumber = randomEngine_() % (maxNote - minNote + 1) + minNote;
    int velocity = randomEngine_() % 60 + 40; // 40-99

    double durationInBeats = 1.0; // Fixed duration for now
    int durationInSamples = static_cast<int>(durationInBeats * (60.0 / bpm) * sampleRate);

    int samplePos = 0; // Simplified position

    midiMessages.addEvent(juce::MidiMessage::noteOn(channel, noteNumber, (juce::uint8)velocity), samplePos);
    midiMessages.addEvent(juce::MidiMessage::noteOff(channel, noteNumber), durationInSamples);
}

void RandomGeneratorV2::setScale(int rootNote, const std::vector<int>& scaleNotes)
{
    // This generator does not currently use scales in its logic,
    // but the method must be implemented.
    juce::ignoreUnused(rootNote, scaleNotes);
}

juce::MidiBuffer RandomGeneratorV2::getPattern(double durationInBeats, juce::AudioProcessorValueTreeState& apvts, double sampleRate)
{
    juce::MidiBuffer pattern;
    double virtualBeat = 0.0;
    const double bpm = *apvts.getRawParameterValue("BPM");
    const double beatsPerSample = bpm / 60.0 / sampleRate;

    // Reset state to ensure predictable pattern generation
    lastBeat_ = -1.0;
    nextEventBeat_ = 0.0;

    while (virtualBeat < durationInBeats)
    {
        process(pattern, apvts, sampleRate, virtualBeat);
        // The process method internally advances lastBeat_, so we just need to update our virtualBeat
        virtualBeat = lastBeat_;
    }

    // Correct sample positions to be relative to the start of the buffer
    juce::MidiBuffer finalPattern;
    int firstSamplePos = -1;

    for (const auto metadata : pattern)
    {
        if (firstSamplePos < 0)
            firstSamplePos = metadata.samplePosition;
        finalPattern.addEvent(metadata.getMessage(), metadata.samplePosition - firstSamplePos);
    }

    return finalPattern;
}
