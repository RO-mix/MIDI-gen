#include "DualEuclideanGenerator.h"

DualEuclideanGenerator::DualEuclideanGenerator()
{
    random_.setSeed(juce::Time::currentTimeMillis());
    updatePattern(patternA_, 16, 4);
    updatePattern(patternB_, 15, 4);
}

void DualEuclideanGenerator::process(juce::MidiBuffer& midiMessages,
                                   juce::AudioProcessorValueTreeState& apvts,
                                   double sampleRate,
                                   double currentBeat)
{
    // Fetch global parameters
    int channel = *apvts.getRawParameterValue("MIDI_CHANNEL");
    double bpm = *apvts.getRawParameterValue("BPM");
    float noteProbability = *apvts.getRawParameterValue("DUAL_EUCLIDEAN_NOTE_PROBABILITY");
    int rateChoice = *apvts.getRawParameterValue("DUAL_EUCLIDEAN_RATE");

    // Fetch params for machine A
    float stepsA_raw = *apvts.getRawParameterValue("DUAL_EUCLIDEAN_STEPS_A");
    int stepsA = static_cast<int>(stepsA_raw);
    float pulsesA_raw = *apvts.getRawParameterValue("DUAL_EUCLIDEAN_PULSES_A");
    int pulsesA = static_cast<int>(pulsesA_raw);
    juce::Logger::writeToLog("Type conversion debug A: stepsA_raw=" + juce::String(stepsA_raw) + ", cast=" + juce::String(stepsA) +
                             ", pulsesA_raw=" + juce::String(pulsesA_raw) + ", cast=" + juce::String(pulsesA));
    int noteA = static_cast<int>(*apvts.getRawParameterValue("DUAL_EUCLIDEAN_NOTE_A"));
    int velocityA = static_cast<int>(*apvts.getRawParameterValue("DUAL_EUCLIDEAN_VELOCITY_A"));
    int devA = static_cast<int>(*apvts.getRawParameterValue("DUAL_EUCLIDEAN_DEVIATION_A"));
    bool bipolarA = *apvts.getRawParameterValue("DUAL_EUCLIDEAN_BIPOLAR_A") > 0.5f;
    float durationBiasA = *apvts.getRawParameterValue("DUAL_EUCLIDEAN_DURATION_BIAS_A");

    // Fetch params for machine B
    int stepsB = static_cast<int>(*apvts.getRawParameterValue("DUAL_EUCLIDEAN_STEPS_B"));
    int pulsesB = static_cast<int>(*apvts.getRawParameterValue("DUAL_EUCLIDEAN_PULSES_B"));
    int noteB = static_cast<int>(*apvts.getRawParameterValue("DUAL_EUCLIDEAN_NOTE_B"));
    int velocityB = static_cast<int>(*apvts.getRawParameterValue("DUAL_EUCLIDEAN_VELOCITY_B"));
    int devB = static_cast<int>(*apvts.getRawParameterValue("DUAL_EUCLIDEAN_DEVIATION_B"));
    bool bipolarB = *apvts.getRawParameterValue("DUAL_EUCLIDEAN_BIPOLAR_B") > 0.5f;
    float durationBiasB = *apvts.getRawParameterValue("DUAL_EUCLIDEAN_DURATION_BIAS_B");

    // Update patterns if needed
    if (stepsA != patternA_.size() || pulsesA != std::count(patternA_.begin(), patternA_.end(), true))
        updatePattern(patternA_, stepsA, pulsesA);
    if (stepsB != patternB_.size() || pulsesB != std::count(patternB_.begin(), patternB_.end(), true))
        updatePattern(patternB_, stepsB, pulsesB);

    // Map rateChoice to actual beat values
    float rateMap[] = { 16.0f, 8.0f, 4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 0.125f, 0.0625f };
    float rate = (rateChoice >= 0 && rateChoice < std::size(rateMap)) ? rateMap[rateChoice] : 0.25f;

    if (lastBeat_ < 0) lastBeat_ = currentBeat;

    while (lastBeat_ < currentBeat)
    {
        if (random_.nextFloat() < noteProbability)
        {
            // Machine A
            if (!patternA_.empty())
            {
                masterStepA_ = (masterStepA_ + 1) % patternA_.size();
                if (patternA_[masterStepA_])
                {
                    int finalNoteA = getDeviatedNote(noteA, devA, bipolarA, lastDeviationA_);
                    float durationInBeats = getRandomDuration(durationBiasA);
                    int durationInSamples = static_cast<int>(durationInBeats * (60.0 / bpm) * sampleRate);
                    int samplePos = static_cast<int>(((lastBeat_ - currentBeat) * (60.0 / bpm)) * sampleRate);
                    if (samplePos < 0) samplePos = 0;
                    midiMessages.addEvent(juce::MidiMessage::noteOn(channel, finalNoteA, (juce::uint8)velocityA), samplePos);
                    midiMessages.addEvent(juce::MidiMessage::noteOff(channel, finalNoteA), samplePos + durationInSamples);
                }
            }

            // Machine B
            if (!patternB_.empty())
            {
                masterStepB_ = (masterStepB_ + 1) % patternB_.size();
                if (patternB_[masterStepB_])
                {
                    int finalNoteB = getDeviatedNote(noteB, devB, bipolarB, lastDeviationB_);
                    float durationInBeats = getRandomDuration(durationBiasB);
                    int durationInSamples = static_cast<int>(durationInBeats * (60.0 / bpm) * sampleRate);
                    int samplePos = static_cast<int>(((lastBeat_ - currentBeat) * (60.0 / bpm)) * sampleRate);
                    if (samplePos < 0) samplePos = 0;
                    midiMessages.addEvent(juce::MidiMessage::noteOn(channel, finalNoteB, (juce::uint8)velocityB), samplePos);
                    midiMessages.addEvent(juce::MidiMessage::noteOff(channel, finalNoteB), samplePos + durationInSamples);
                }
            }
        }
        lastBeat_ += rate;
    }
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
    juce::MidiBuffer pattern;
    double virtualBeat = 0.0;
    const double bpm = *apvts.getRawParameterValue("BPM");
    [[maybe_unused]] const double beatsPerSample = bpm / 60.0 / sampleRate;

    // Reset state to ensure predictable pattern generation
    lastBeat_ = -1.0;
    masterStepA_ = -1;
    masterStepB_ = -1;

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

int DualEuclideanGenerator::getDeviatedNote(int baseNote, int deviationRange, bool isBipolar, int& lastDeviation)
{
    if (deviationRange > 0 && !scaleNotes_.empty())
    {
        int step = (random_.nextInt(3) - 1);
        lastDeviation += step;

        if (isBipolar)
            lastDeviation = juce::jlimit(-deviationRange, deviationRange, lastDeviation);
        else
            lastDeviation = juce::jlimit(0, deviationRange, lastDeviation);

        auto it = std::find_if(scaleNotes_.begin(), scaleNotes_.end(), [&](int scaleNote){ return (baseNote % 12) == (rootNote_ + scaleNote) % 12; });
        if(it != scaleNotes_.end())
        {
            int baseIndex = std::distance(scaleNotes_.begin(), it);
            int newIndex = baseIndex + lastDeviation;

            while(newIndex < 0) newIndex += scaleNotes_.size();
            newIndex %= scaleNotes_.size();

            int octave = baseNote / 12;
            return juce::jlimit(0, 127, (octave * 12) + rootNote_ + scaleNotes_[newIndex]);
        }
    }
    return baseNote;
}

float DualEuclideanGenerator::getRandomDuration(float bias)
{
    // Using the same musical duration logic from RandomGenerator
    float r1 = random_.nextFloat();
    float r2 = random_.nextFloat();
    float bates = (r1 + r2) / 2.0f;
    float biased_center = juce::jmap(bias, 0.0f, 1.0f, 2.0f, 0.1f);
    float range = 1.5f;
    float duration = biased_center + (bates - 0.5f) * range;
    return juce::jlimit(0.05f, 4.0f, duration);
}