#include "RandomGenerator.h"
#include "../Theory/Scales.h"
#include "../Theory/Duration.h"

RandomGenerator::RandomGenerator()
    : distribution_(0.0f, 1.0f)
{
    std::random_device rd;
    randomEngine_.seed(rd());
}

juce::Array<PendingNoteOff> RandomGenerator::process(juce::MidiBuffer& midiMessages,
                                                     juce::AudioProcessorValueTreeState& apvts,
                                                     double sampleRate,
                                                     double blockStartTime,
                                                     double blockEndTime,
                                                     int numSamples)
{
    juce::ignoreUnused(numSamples);
    // Fetch parameters from APVTS
    auto* minNoteParam = apvts.getRawParameterValue("RANDOM_MIN_NOTE");
    auto* maxNoteParam = apvts.getRawParameterValue("RANDOM_MAX_NOTE");
    auto* maxVelocityParam = apvts.getRawParameterValue("RANDOM_MAX_VELOCITY");
    auto* velocityBiasParam = apvts.getRawParameterValue("RANDOM_VELOCITY_BIAS");
    auto* noteProbabilityParam = apvts.getRawParameterValue("RANDOM_NOTE_PROBABILITY");
    auto* durationBiasParam = apvts.getRawParameterValue("RANDOM_DURATION_BIAS");
    auto* rateParam = apvts.getRawParameterValue("RANDOM_RATE"); // This is a choice parameter
    auto* addCC74Param = apvts.getRawParameterValue("RANDOM_ADD_CC74");
    auto* channelParam = apvts.getRawParameterValue("MIDI_CHANNEL");
    auto* bpmParam = apvts.getRawParameterValue("BPM");


    if (!minNoteParam || !maxNoteParam || !maxVelocityParam || !velocityBiasParam ||
        !noteProbabilityParam || !durationBiasParam || !rateParam || !addCC74Param || !channelParam || !bpmParam)
    {
        // One or more parameters not found, abort.
        return {};
    }

    int minNote = static_cast<int>(*minNoteParam);
    int maxNote = static_cast<int>(*maxNoteParam);
    int maxVelocity = static_cast<int>(*maxVelocityParam);
    float velocityBias = velocityBiasParam->load();
    float noteProbability = noteProbabilityParam->load();
    float durationBias = durationBiasParam->load();
    int rateChoice = static_cast<int>(rateParam->load());
    bool addCC74 = *addCC74Param > 0.5f;
    int channel = static_cast<int>(*channelParam);
    double bpm = *bpmParam;

    // Map rateChoice to actual beat values
    float rateMap[] = { 16.0f, 8.0f, 4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 0.125f, 0.0625f };
    float rate = (rateChoice >= 0 && rateChoice < std::size(rateMap)) ? rateMap[rateChoice] : 1.0f;

    if (lastBeat_ < 0) lastBeat_ = blockStartTime;

    // Process events for the current block
    double blockDurationBeats = blockEndTime - blockStartTime;
    int startSample = 0; // We process from the start of the block
    while (lastBeat_ < blockEndTime)
    {
        if (distribution_(randomEngine_) < noteProbability)
        {
            if (minNote <= maxNote)
            {
                // Generate a note
                int noteNumber;
                if (scaleNotes_.empty())
                {
                    // Chromatic
                    noteNumber = minNote + (randomEngine_() % (maxNote - minNote + 1));
                }
            else
            {
                // In scale
                int noteIndex = randomEngine_() % scaleNotes_.size();
                // Find a suitable octave
                int baseNote = rootNote_ + scaleNotes_[noteIndex];
                int octave = (minNote - baseNote) / 12;
                noteNumber = baseNote + octave * 12;
                while(noteNumber < minNote) noteNumber += 12;

                std::vector<int> possibleNotes;
                while(noteNumber <= maxNote)
                {
                    possibleNotes.push_back(noteNumber);
                    noteNumber += 12;
                }
                if(possibleNotes.empty())
                {
                    lastBeat_ += rate;
                    continue; // No notes in range
                }
                noteNumber = possibleNotes[randomEngine_() % possibleNotes.size()];
            }

            int velocity = calculateVelocity(velocityBias, maxVelocity);
            float durationInBeats = Duration::getBiasedDuration(durationBias, rate);
            int durationInSamples = static_cast<int>(durationInBeats * (60.0 / bpm) * sampleRate);

            // Calculate sample position for the note on event
            double beatInBlock = lastBeat_ - blockStartTime;
            int samplePos = static_cast<int>((beatInBlock / blockDurationBeats) * numSamples);
            if (samplePos < 0) samplePos = startSample; // Ensure it's within the current block

            if (samplePos < numSamples)
            {
                midiMessages.addEvent(juce::MidiMessage::noteOn(channel, noteNumber, (juce::uint8)velocity), samplePos);
                midiMessages.addEvent(juce::MidiMessage::noteOff(channel, noteNumber), samplePos + durationInSamples);

                if (addCC74)
                {
                    midiMessages.addEvent(juce::MidiMessage::controllerEvent(channel, 74, randomEngine_() % 128), samplePos);
                }
            }
            }
        }

        lastBeat_ += rate;
    }
    // lastBeat_ is now correctly preserved across blocks.
    return {};
}

void RandomGenerator::setScale(int rootNote, const std::vector<int>& scaleNotes)
{
    rootNote_ = rootNote;
    scaleNotes_ = scaleNotes;
}

int RandomGenerator::calculateVelocity(float bias, int maxVelocity)
{
    float alpha, beta;
    float inverted_bias = 1.0f - bias;
    if (inverted_bias < 0.5f)
    {
        alpha = 1.0f + (0.5f - inverted_bias) * 8.0f;
        beta = 1.0f;
    }
    else
    {
        alpha = 1.0f;
        beta = 1.0f + (inverted_bias - 0.5f) * 8.0f;
    }

    std::gamma_distribution<float> dist1(alpha, 1.0f);
    std::gamma_distribution<float> dist2(beta, 1.0f);

    float g1 = dist1(randomEngine_);
    float g2 = dist2(randomEngine_);

    // Avoid division by zero if both are somehow zero
    if (g1 + g2 == 0.0f) return 1;

    float random_val = g1 / (g1 + g2);

    int random_velocity = 1 + static_cast<int>(random_val * (maxVelocity - 1));

    return juce::jlimit(1, maxVelocity, random_velocity);
}

void RandomGenerator::reset()
{
    lastBeat_ = -1.0;
}

juce::MidiBuffer RandomGenerator::getPattern(double durationInBeats, juce::AudioProcessorValueTreeState& apvts, double sampleRate)
{
    juce::MidiBuffer pattern;
    double currentBeat = 0.0;

    // Fetch parameters from APVTS
    auto* minNoteParam = apvts.getRawParameterValue("RANDOM_MIN_NOTE");
    auto* maxNoteParam = apvts.getRawParameterValue("RANDOM_MAX_NOTE");
    auto* maxVelocityParam = apvts.getRawParameterValue("RANDOM_MAX_VELOCITY");
    auto* velocityBiasParam = apvts.getRawParameterValue("RANDOM_VELOCITY_BIAS");
    auto* noteProbabilityParam = apvts.getRawParameterValue("RANDOM_NOTE_PROBABILITY");
    auto* durationBiasParam = apvts.getRawParameterValue("RANDOM_DURATION_BIAS");
    auto* rateParam = apvts.getRawParameterValue("RANDOM_RATE");
    auto* addCC74Param = apvts.getRawParameterValue("RANDOM_ADD_CC74");
    auto* channelParam = apvts.getRawParameterValue("MIDI_CHANNEL");
    auto* bpmParam = apvts.getRawParameterValue("BPM");

    if (!minNoteParam || !maxNoteParam || !maxVelocityParam || !velocityBiasParam ||
        !noteProbabilityParam || !durationBiasParam || !rateParam || !addCC74Param || !channelParam || !bpmParam)
    {
        return pattern; // Return empty buffer if params are missing
    }

    int minNote = static_cast<int>(minNoteParam->load());
    int maxNote = static_cast<int>(maxNoteParam->load());
    int maxVelocity = static_cast<int>(maxVelocityParam->load());
    float velocityBias = velocityBiasParam->load();
    float noteProbability = noteProbabilityParam->load();
    float durationBias = durationBiasParam->load();
    int rateChoice = static_cast<int>(rateParam->load());
    bool addCC74 = addCC74Param->load() > 0.5f;
    int channel = static_cast<int>(*channelParam);
    double bpm = *bpmParam;

    float rateMap[] = { 16.0f, 8.0f, 4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 0.125f, 0.0625f };
    float rate = (rateChoice >= 0 && rateChoice < std::size(rateMap)) ? rateMap[rateChoice] : 1.0f;

    while (currentBeat < durationInBeats)
    {
        if (distribution_(randomEngine_) < noteProbability)
        {
            if (minNote <= maxNote)
            {
                int noteNumber;
                if (scaleNotes_.empty())
                {
                    noteNumber = minNote + (randomEngine_() % (maxNote - minNote + 1));
                }
            else
            {
                int noteIndex = randomEngine_() % scaleNotes_.size();
                int baseNote = rootNote_ + scaleNotes_[noteIndex];
                int octave = (minNote - baseNote) / 12;
                noteNumber = baseNote + octave * 12;
                while(noteNumber < minNote) noteNumber += 12;

                std::vector<int> possibleNotes;
                while(noteNumber <= maxNote)
                {
                    possibleNotes.push_back(noteNumber);
                    noteNumber += 12;
                }

                if (!possibleNotes.empty())
                {
                    noteNumber = possibleNotes[randomEngine_() % possibleNotes.size()];
                }
                else
                {
                    currentBeat += rate;
                    continue; // No notes in range, skip to next beat
                }
            }

            int velocity = calculateVelocity(velocityBias, maxVelocity);
            float duration = Duration::getBiasedDuration(durationBias, rate);
            int samplePos = static_cast<int>(currentBeat * (60.0 / bpm) * sampleRate);
            int durationInSamples = static_cast<int>(duration * (60.0 / bpm) * sampleRate);

            pattern.addEvent(juce::MidiMessage::noteOn(channel, noteNumber, (juce::uint8)velocity), samplePos);
            pattern.addEvent(juce::MidiMessage::noteOff(channel, noteNumber), samplePos + durationInSamples);

            if (addCC74)
            {
                pattern.addEvent(juce::MidiMessage::controllerEvent(channel, 74, randomEngine_() % 128), samplePos);
            }
            }
        }
        currentBeat += rate;
    }

    return pattern;
}