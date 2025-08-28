#include "DualEuclideanGenerator.h"
#include "../Theory/Duration.h"

DualEuclideanGenerator::DualEuclideanGenerator()
{
    random_.setSeed(juce::Time::currentTimeMillis());
    updatePattern(patternA_, 16, 4);
    updatePattern(patternB_, 15, 4);
}

juce::Array<PendingNoteOff> DualEuclideanGenerator::process(juce::MidiBuffer& midiMessages,
                                                            juce::AudioProcessorValueTreeState& apvts,
                                                            double sampleRate,
                                                            double blockStartTime,
                                                            double blockEndTime,
                                                            int numSamples)
{
    juce::ignoreUnused(numSamples);
    // Fetch global parameters
    int channel = static_cast<int>(apvts.getRawParameterValue("MIDI_CHANNEL")->load());
    double bpm = apvts.getRawParameterValue("BPM")->load();
    float noteProbability = apvts.getRawParameterValue("DUAL_EUCLIDEAN_NOTE_PROBABILITY")->load();
    int rateChoice = static_cast<int>(apvts.getRawParameterValue("DUAL_EUCLIDEAN_RATE")->load());

    // Fetch params for machine A
    int stepsA = static_cast<int>(apvts.getRawParameterValue("DUAL_EUCLIDEAN_STEPS_A")->load());
    int pulsesA = static_cast<int>(apvts.getRawParameterValue("DUAL_EUCLIDEAN_PULSES_A")->load());
    int noteA = static_cast<int>(apvts.getRawParameterValue("DUAL_EUCLIDEAN_NOTE_A")->load());
    int velocityA = static_cast<int>(apvts.getRawParameterValue("DUAL_EUCLIDEAN_VELOCITY_A")->load());
    int devA = static_cast<int>(apvts.getRawParameterValue("DUAL_EUCLIDEAN_DEVIATION_A")->load());
    bool bipolarA = apvts.getRawParameterValue("DUAL_EUCLIDEAN_BIPOLAR_A")->load() > 0.5f;
    float durationBiasA = apvts.getRawParameterValue("DUAL_EUCLIDEAN_DURATION_BIAS_A")->load();

    // Fetch params for machine B
    int stepsB = static_cast<int>(apvts.getRawParameterValue("DUAL_EUCLIDEAN_STEPS_B")->load());
    int pulsesB = static_cast<int>(apvts.getRawParameterValue("DUAL_EUCLIDEAN_PULSES_B")->load());
    int noteB = static_cast<int>(apvts.getRawParameterValue("DUAL_EUCLIDEAN_NOTE_B")->load());
    int velocityB = static_cast<int>(apvts.getRawParameterValue("DUAL_EUCLIDEAN_VELOCITY_B")->load());
    int devB = static_cast<int>(apvts.getRawParameterValue("DUAL_EUCLIDEAN_DEVIATION_B")->load());
    bool bipolarB = apvts.getRawParameterValue("DUAL_EUCLIDEAN_BIPOLAR_B")->load() > 0.5f;
    float durationBiasB = apvts.getRawParameterValue("DUAL_EUCLIDEAN_DURATION_BIAS_B")->load();

    // Update patterns if needed
    if (stepsA != static_cast<int>(patternA_.size()) || pulsesA != std::count(patternA_.begin(), patternA_.end(), true))
        updatePattern(patternA_, stepsA, pulsesA);
    if (stepsB != static_cast<int>(patternB_.size()) || pulsesB != std::count(patternB_.begin(), patternB_.end(), true))
        updatePattern(patternB_, stepsB, pulsesB);

    // Map rateChoice to actual beat values
    float rateMap[] = { 16.0f, 8.0f, 4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 0.125f, 0.0625f };
    float rate = (rateChoice >= 0 && static_cast<size_t>(rateChoice) < std::size(rateMap)) ? rateMap[rateChoice] : 0.25f;

    if (lastBeat_ < 0) lastBeat_ = blockStartTime;
    double blockDurationBeats = blockEndTime - blockStartTime;

    while (lastBeat_ < blockEndTime)
    {
        if (random_.nextFloat() < noteProbability)
        {
            // Machine A
            if (!patternA_.empty())
            {
                masterStepA_ = (masterStepA_ + 1) % patternA_.size();
                if (patternA_[masterStepA_])
                {
                    int finalNoteA = getDeviatedNote(noteA, devA, bipolarA);
                    float durationInBeats = Duration::getBiasedDuration(durationBiasA, rate);
                    int durationInSamples = static_cast<int>(durationInBeats * (60.0 / bpm) * sampleRate);
                    double beatInBlock = lastBeat_ - blockStartTime;
                    int samplePos = static_cast<int>((beatInBlock / blockDurationBeats) * numSamples);
                    if (samplePos < 0) samplePos = 0;
                    if (samplePos < numSamples)
                    {
                        midiMessages.addEvent(juce::MidiMessage::noteOn(channel, finalNoteA, (juce::uint8)velocityA), samplePos);
                        midiMessages.addEvent(juce::MidiMessage::noteOff(channel, finalNoteA), samplePos + durationInSamples);
                    }
                }
            }

            // Machine B
            if (!patternB_.empty())
            {
                masterStepB_ = (masterStepB_ + 1) % patternB_.size();
                if (patternB_[masterStepB_])
                {
                    int finalNoteB = getDeviatedNote(noteB, devB, bipolarB);
                    float durationInBeats = Duration::getBiasedDuration(durationBiasB, rate);
                    int durationInSamples = static_cast<int>(durationInBeats * (60.0 / bpm) * sampleRate);
                    double beatInBlock = lastBeat_ - blockStartTime;
                    int samplePos = static_cast<int>((beatInBlock / blockDurationBeats) * numSamples);
                    if (samplePos < 0) samplePos = 0;
                    if (samplePos < numSamples)
                    {
                        midiMessages.addEvent(juce::MidiMessage::noteOn(channel, finalNoteB, (juce::uint8)velocityB), samplePos);
                        midiMessages.addEvent(juce::MidiMessage::noteOff(channel, finalNoteB), samplePos + durationInSamples);
                    }
                }
            }
        }
        lastBeat_ += rate;
    }
    // lastBeat_ is now correctly preserved across blocks.
    return {};
}

void DualEuclideanGenerator::updatePattern(std::vector<bool>& pattern, int steps, int pulses)
{
    steps = juce::jmax(1, steps);
    pulses = juce::jlimit(0, steps, pulses);

    pattern.assign(steps, false);
    if (pulses == 0) return;

    int bucket = 0;
    for (int i = 0; i < steps; ++i)
    {
        bucket += pulses;
        if (bucket >= steps)
        {
            bucket -= steps;
            pattern[i] = true;
        }
    }
}

void DualEuclideanGenerator::setScale(int rootNote, const std::vector<int>& scaleNotes)
{
    rootNote_ = rootNote;
    scaleNotes_ = scaleNotes;
}

juce::MidiBuffer DualEuclideanGenerator::getPattern(double durationInBeats, juce::AudioProcessorValueTreeState& apvts, double sampleRate)
{
    juce::MidiBuffer patternBuffer;
    double currentBeat = 0.0;

    // Fetch global parameters
    int channel = static_cast<int>(apvts.getRawParameterValue("MIDI_CHANNEL")->load());
    double bpm = apvts.getRawParameterValue("BPM")->load();
    float noteProbability = apvts.getRawParameterValue("DUAL_EUCLIDEAN_NOTE_PROBABILITY")->load();
    int rateChoice = static_cast<int>(apvts.getRawParameterValue("DUAL_EUCLIDEAN_RATE")->load());

    // Fetch params for machine A
    int stepsA = static_cast<int>(apvts.getRawParameterValue("DUAL_EUCLIDEAN_STEPS_A")->load());
    int pulsesA = static_cast<int>(apvts.getRawParameterValue("DUAL_EUCLIDEAN_PULSES_A")->load());
    int noteA = static_cast<int>(apvts.getRawParameterValue("DUAL_EUCLIDEAN_NOTE_A")->load());
    int velocityA = static_cast<int>(apvts.getRawParameterValue("DUAL_EUCLIDEAN_VELOCITY_A")->load());
    int devA = static_cast<int>(apvts.getRawParameterValue("DUAL_EUCLIDEAN_DEVIATION_A")->load());
    bool bipolarA = apvts.getRawParameterValue("DUAL_EUCLIDEAN_BIPOLAR_A")->load() > 0.5f;
    float durationBiasA = apvts.getRawParameterValue("DUAL_EUCLIDEAN_DURATION_BIAS_A")->load();

    // Fetch params for machine B
    int stepsB = static_cast<int>(apvts.getRawParameterValue("DUAL_EUCLIDEAN_STEPS_B")->load());
    int pulsesB = static_cast<int>(apvts.getRawParameterValue("DUAL_EUCLIDEAN_PULSES_B")->load());
    int noteB = static_cast<int>(apvts.getRawParameterValue("DUAL_EUCLIDEAN_NOTE_B")->load());
    int velocityB = static_cast<int>(apvts.getRawParameterValue("DUAL_EUCLIDEAN_VELOCITY_B")->load());
    int devB = static_cast<int>(apvts.getRawParameterValue("DUAL_EUCLIDEAN_DEVIATION_B")->load());
    bool bipolarB = apvts.getRawParameterValue("DUAL_EUCLIDEAN_BIPOLAR_B")->load() > 0.5f;
    float durationBiasB = apvts.getRawParameterValue("DUAL_EUCLIDEAN_DURATION_BIAS_B")->load();

    updatePattern(patternA_, stepsA, pulsesA);
    updatePattern(patternB_, stepsB, pulsesB);

    float rateMap[] = { 16.0f, 8.0f, 4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 0.125f, 0.0625f };
    float rate = (rateChoice >= 0 && static_cast<size_t>(rateChoice) < std::size(rateMap)) ? rateMap[rateChoice] : 0.25f;

    int stepCounterA = 0;
    int stepCounterB = 0;

    while (currentBeat < durationInBeats)
    {
        if (random_.nextFloat() < noteProbability)
        {
            if (!patternA_.empty() && patternA_[stepCounterA % stepsA])
            {
                int finalNoteA = getDeviatedNote(noteA, devA, bipolarA);
                float durationInBeatsA = Duration::getBiasedDuration(durationBiasA, rate);
                int durationInSamplesA = static_cast<int>(durationInBeatsA * (60.0 / bpm) * sampleRate);
                int samplePos = static_cast<int>(currentBeat * (60.0 / bpm) * sampleRate);
                patternBuffer.addEvent(juce::MidiMessage::noteOn(channel, finalNoteA, (juce::uint8)velocityA), samplePos);
                patternBuffer.addEvent(juce::MidiMessage::noteOff(channel, finalNoteA), samplePos + durationInSamplesA);
            }

            if (!patternB_.empty() && patternB_[stepCounterB % stepsB])
            {
                int finalNoteB = getDeviatedNote(noteB, devB, bipolarB);
                float durationInBeatsB = Duration::getBiasedDuration(durationBiasB, rate);
                int durationInSamplesB = static_cast<int>(durationInBeatsB * (60.0 / bpm) * sampleRate);
                int samplePos = static_cast<int>(currentBeat * (60.0 / bpm) * sampleRate);
                patternBuffer.addEvent(juce::MidiMessage::noteOn(channel, finalNoteB, (juce::uint8)velocityB), samplePos);
                patternBuffer.addEvent(juce::MidiMessage::noteOff(channel, finalNoteB), samplePos + durationInSamplesB);
            }
        }
        currentBeat += rate;
        stepCounterA++;
        stepCounterB++;
    }

    return patternBuffer;
}

int DualEuclideanGenerator::getDeviatedNote(int baseNote, int deviationRange, bool isBipolar)
{
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
                minNote = baseNote - deviationRange;
                maxNote = baseNote + deviationRange;
            }
            else
            {
                minNote = baseNote;
                maxNote = baseNote + deviationRange;
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
                return notesInRange[random_.nextInt(notesInRange.size())];
            }
        }
    }
    return baseNote;
}

void DualEuclideanGenerator::reset()
{
    lastBeat_ = -1.0;
    masterStepA_ = -1;
    masterStepB_ = -1;
    lastDeviationA_ = 0;
    lastDeviationB_ = 0;
}