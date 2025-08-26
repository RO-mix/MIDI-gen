#include "RandomGenerator.h"
#include "../Theory/Scales.h"

RandomGenerator::RandomGenerator()
    : distribution_(0.0f, 1.0f)
{
    std::random_device rd;
    randomEngine_.seed(rd());
}

void RandomGenerator::process(juce::MidiBuffer& midiMessages,
                            juce::AudioProcessorValueTreeState& apvts,
                            double sampleRate,
                            double currentBeat)
{
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
        return;
    }

    int minNote = static_cast<int>(*minNoteParam);
    int maxNote = static_cast<int>(*maxNoteParam);
    int maxVelocity = static_cast<int>(*maxVelocityParam);
    float velocityBias = *velocityBiasParam;
    float noteProbability = *noteProbabilityParam;
    float durationBias = *durationBiasParam;
    int rateChoice = static_cast<int>(*rateParam);
    bool addCC74 = *addCC74Param > 0.5f;
    int channel = static_cast<int>(*channelParam);
    double bpm = *bpmParam;

    // Map rateChoice to actual beat values
    float rateMap[] = { 16.0f, 8.0f, 4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 0.125f, 0.0625f };
    float rate = (rateChoice >= 0 && rateChoice < std::size(rateMap)) ? rateMap[rateChoice] : 1.0f;

    if (lastBeat_ < 0) lastBeat_ = currentBeat;

    // Process events for the current block
    int startSample = 0; // We process from the start of the block
    while (lastBeat_ < currentBeat)
    {
        if (distribution_(randomEngine_) < noteProbability)
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
                if(possibleNotes.empty()) continue; // No notes in range
                noteNumber = possibleNotes[randomEngine_() % possibleNotes.size()];
            }

            int velocity = calculateVelocity(velocityBias, maxVelocity);
            float durationInBeats = getRandomDuration(durationBias);
            int durationInSamples = static_cast<int>(durationInBeats * (60.0 / bpm) * sampleRate);

            // Calculate sample position for the note on event
            int samplePos = static_cast<int>(((lastBeat_ - currentBeat) / (60.0 / bpm)) * sampleRate);
            if (samplePos < 0) samplePos = startSample; // Ensure it's within the current block

            midiMessages.addEvent(juce::MidiMessage::noteOn(channel, noteNumber, (juce::uint8)velocity), samplePos);
            midiMessages.addEvent(juce::MidiMessage::noteOff(channel, noteNumber), samplePos + durationInSamples);

            if (addCC74)
            {
                midiMessages.addEvent(juce::MidiMessage::controllerEvent(channel, 74, randomEngine_() % 128), samplePos);
            }
        }

        lastBeat_ += rate;
    }
}

void RandomGenerator::setScale(int rootNote, const std::vector<int>& scaleNotes)
{
    rootNote_ = rootNote;
    scaleNotes_ = scaleNotes;
}

int RandomGenerator::calculateVelocity(float bias, int maxVelocity)
{
    // A more musical velocity curve. Bias towards the extremes is less linear.
    float r = distribution_(randomEngine_);
    float skewed_r = juce::jmap(r, 0.0f, 1.0f, 1.0f - bias, bias);
    float curved_r = std::pow(skewed_r, 2.0f); // Exponential curve

    // Add some slight random variation
    float variation = 1.0f + (distribution_(randomEngine_) - 0.5f) * 0.2f; // +/- 10%

    return juce::jlimit(1, maxVelocity, static_cast<int>(curved_r * maxVelocity * variation));
}

float RandomGenerator::getRandomDuration(float bias)
{
    // More musical duration. Bias affects the center of a distribution.
    float r1 = distribution_(randomEngine_);
    float r2 = distribution_(randomEngine_);

    // Bates distribution (sum of uniform distributions) to create a more normal-like curve
    float bates = (r1 + r2) / 2.0f;

    // Bias shifts the center of this distribution
    float biased_center = juce::jmap(bias, 0.0f, 1.0f, 2.0f, 0.1f); // Long to short
    float range = 1.5f;

    float duration = biased_center + (bates - 0.5f) * range;

    return juce::jlimit(0.05f, 4.0f, duration); // Limit duration from a 64th note to a whole note
}

juce::MidiBuffer RandomGenerator::getPattern(double durationInBeats, juce::AudioProcessorValueTreeState& apvts, double sampleRate)
{
    juce::MidiBuffer pattern;
    double virtualBeat = 0.0;
    const double bpm = *apvts.getRawParameterValue("BPM");
    const double beatsPerSample = bpm / 60.0 / sampleRate;

    // Reset state to ensure predictable pattern generation
    lastBeat_ = -1.0;

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