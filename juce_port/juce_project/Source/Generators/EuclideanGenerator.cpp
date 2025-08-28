#include "EuclideanGenerator.h"
#include "../Theory/Duration.h"

EuclideanGenerator::EuclideanGenerator()
{
    random_.setSeed(juce::Time::currentTimeMillis());
    updatePattern(16, 4); // Default values
}

juce::Array<PendingNoteOff> EuclideanGenerator::process(juce::MidiBuffer& midiMessages,
                                                        juce::AudioProcessorValueTreeState& apvts,
                                                        double sampleRate,
                                                        double blockStartTime,
                                                        double blockEndTime,
                                                        int numSamples)
{
    juce::ignoreUnused(numSamples);
    recentNotes.clear();

    // Fetch parameters
    int steps = *apvts.getRawParameterValue("EUCLIDEAN_STEPS");
    int pulses = *apvts.getRawParameterValue("EUCLIDEAN_PULSES");
    int note = *apvts.getRawParameterValue("EUCLIDEAN_NOTE");
    int velocity = *apvts.getRawParameterValue("EUCLIDEAN_VELOCITY");
    int deviationRange = *apvts.getRawParameterValue("EUCLIDEAN_DEVIATION_RANGE");
    bool isBipolar = *apvts.getRawParameterValue("EUCLIDEAN_DEVIATION_BIPOLAR") > 0.5f;
    int rateChoice = *apvts.getRawParameterValue("EUCLIDEAN_RATE");
    float noteProbability = *apvts.getRawParameterValue("EUCLIDEAN_NOTE_PROBABILITY");
    auto* durationBiasParam = apvts.getRawParameterValue("EUCLIDEAN_DURATION_BIAS");
    int channel = *apvts.getRawParameterValue("MIDI_CHANNEL");
    double bpm = *apvts.getRawParameterValue("BPM");

    // Update pattern if needed
    if (steps != (int)pattern_.size() || pulses != std::count(pattern_.begin(), pattern_.end(), true))
    {
        updatePattern(steps, pulses);
    }
    if (pattern_.empty()) return {};


    // Map rateChoice to actual beat values
    float rateMap[] = { 16.0f, 8.0f, 4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 0.125f, 0.0625f };
    float rate = (rateChoice >= 0 && rateChoice < std::size(rateMap)) ? rateMap[rateChoice] : 0.25f;

    if (lastBeat_ < 0) lastBeat_ = blockStartTime;

    double blockDurationBeats = blockEndTime - blockStartTime;

    while (lastBeat_ < blockEndTime)
    {
        currentStep_ = (currentStep_ + 1) % steps;

        if (pattern_[currentStep_] && random_.nextFloat() < noteProbability)
        {
            int generatedNote = note;
            if (deviationRange > 0 && !scaleNotes_.empty())
            {
                std::vector<int> possibleNotes;
                for (int i = 0; i < 128; ++i)
                {
                    bool isInScale = false;
                    for (int scaleNote : scaleNotes_)
                    {
                        if (i % 12 == (rootNote_ + scaleNote) % 12)
                        {
                            isInScale = true;
                            break;
                        }
                    }
                    if (isInScale)
                    {
                        possibleNotes.push_back(i);
                    }
                }

                if (!possibleNotes.empty())
                {
                    int minNote, maxNote;
                    if (isBipolar)
                    {
                        minNote = note - deviationRange;
                        maxNote = note + deviationRange;
                    }
                    else
                    {
                        minNote = note;
                        maxNote = note + deviationRange;
                    }

                    std::vector<int> notesInRange;
                    for (int p_note : possibleNotes)
                    {
                        if (p_note >= minNote && p_note <= maxNote)
                        {
                            notesInRange.push_back(p_note);
                        }
                    }

                    if (!notesInRange.empty())
                    {
                        generatedNote = notesInRange[random_.nextInt(notesInRange.size())];
                    }
                }
            }

            generatedNote = juce::jlimit(0, 127, generatedNote);
            float durationInBeats = Duration::getBiasedDuration(durationBiasParam ? durationBiasParam->load() : 0.5f, rate);
            int durationInSamples = static_cast<int>(durationInBeats * (60.0 / bpm) * sampleRate);

            double beatInBlock = lastBeat_ - blockStartTime;
            int samplePos = static_cast<int>((beatInBlock / blockDurationBeats) * numSamples);
            if (samplePos < 0) samplePos = 0;

            if (samplePos < numSamples)
            {
                midiMessages.addEvent(juce::MidiMessage::noteOn(channel, generatedNote, (juce::uint8)velocity), samplePos);
                midiMessages.addEvent(juce::MidiMessage::noteOff(channel, generatedNote), samplePos + durationInSamples);
                recentNotes.push_back({generatedNote, velocity, lastBeat_, durationInBeats});
            }
        }

        lastBeat_ += rate;
    }
    // lastBeat_ is now correctly preserved across blocks.
    return {};
}

void EuclideanGenerator::updatePattern(int steps, int pulses)
{
    steps = juce::jmax(1, steps);
    pulses = juce::jlimit(0, steps, pulses);

    pattern_.assign(steps, false);
    if (pulses == 0) return;

    // Bjorklund's algorithm
    int bucket = 0;
    for (int i = 0; i < steps; ++i)
    {
        bucket += pulses;
        if (bucket >= steps)
        {
            bucket -= steps;
            pattern_[i] = true;
        }
    }
    currentStep_ = -1;
}

void EuclideanGenerator::setScale(int rootNote, const std::vector<int>& scaleNotes)
{
    rootNote_ = rootNote;
    scaleNotes_ = scaleNotes;
}

juce::MidiBuffer EuclideanGenerator::getPattern(double durationInBeats, juce::AudioProcessorValueTreeState& apvts, double sampleRate)
{
    juce::MidiBuffer patternBuffer;
    double currentBeat = 0.0;

    // Fetch parameters
    int steps = *apvts.getRawParameterValue("EUCLIDEAN_STEPS");
    int pulses = *apvts.getRawParameterValue("EUCLIDEAN_PULSES");
    int note = *apvts.getRawParameterValue("EUCLIDEAN_NOTE");
    int velocity = *apvts.getRawParameterValue("EUCLIDEAN_VELOCITY");
    int deviationRange = *apvts.getRawParameterValue("EUCLIDEAN_DEVIATION_RANGE");
    bool isBipolar = *apvts.getRawParameterValue("EUCLIDEAN_DEVIATION_BIPOLAR") > 0.5f;
    int rateChoice = *apvts.getRawParameterValue("EUCLIDEAN_RATE");
    float noteProbability = *apvts.getRawParameterValue("EUCLIDEAN_NOTE_PROBABILITY");
    auto* durationBiasParam = apvts.getRawParameterValue("EUCLIDEAN_DURATION_BIAS");
    int channel = *apvts.getRawParameterValue("MIDI_CHANNEL");
    double bpm = *apvts.getRawParameterValue("BPM");

    updatePattern(steps, pulses);
    if (pattern_.empty()) return patternBuffer;

    float rateMap[] = { 16.0f, 8.0f, 4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 0.125f, 0.0625f };
    float rate = (rateChoice >= 0 && rateChoice < std::size(rateMap)) ? rateMap[rateChoice] : 0.25f;

    int stepCounter = 0;
    lastDeviation_ = 0; // Reset deviation for predictable generation

    while (currentBeat < durationInBeats)
    {
        if (pattern_[stepCounter % steps] && random_.nextFloat() < noteProbability)
        {
            int generatedNote = note;
            if (deviationRange > 0 && !scaleNotes_.empty())
            {
                int devStep = (random_.nextInt(3) - 1);
                lastDeviation_ += devStep;
                if (isBipolar)
                    lastDeviation_ = juce::jlimit(-deviationRange, deviationRange, lastDeviation_);
                else
                    lastDeviation_ = juce::jlimit(0, deviationRange, lastDeviation_);

                auto it = std::find_if(scaleNotes_.begin(), scaleNotes_.end(), [&](int scaleNote){ return (note % 12) == (rootNote_ + scaleNote) % 12; });
                if(it != scaleNotes_.end())
                {
                    int baseIndex = std::distance(scaleNotes_.begin(), it);
                    int newIndex = baseIndex + lastDeviation_;
                    while(newIndex < 0) newIndex += scaleNotes_.size();
                    newIndex %= scaleNotes_.size();
                    int octave = note / 12;
                    generatedNote = (octave * 12) + rootNote_ + scaleNotes_[newIndex];
                }
            }

            generatedNote = juce::jlimit(0, 127, generatedNote);
            float duration = Duration::getBiasedDuration(durationBiasParam ? durationBiasParam->load() : 0.5f, rate);
            int samplePos = static_cast<int>(currentBeat * (60.0 / bpm) * sampleRate);
            int durationInSamples = static_cast<int>(duration * (60.0 / bpm) * sampleRate);

            patternBuffer.addEvent(juce::MidiMessage::noteOn(channel, generatedNote, (juce::uint8)velocity), samplePos);
            patternBuffer.addEvent(juce::MidiMessage::noteOff(channel, generatedNote), samplePos + durationInSamples);
        }

        currentBeat += rate;
        stepCounter++;
    }

    return patternBuffer;
}

void EuclideanGenerator::reset()
{
    lastBeat_ = -1.0;
    currentStep_ = -1;
    lastDeviation_ = 0;
}