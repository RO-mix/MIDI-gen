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
    int stepsA = *apvts.getRawParameterValue("DUAL_EUCLIDEAN_STEPS_A");
    int pulsesA = *apvts.getRawParameterValue("DUAL_EUCLIDEAN_PULSES_A");
    int noteA = *apvts.getRawParameterValue("DUAL_EUCLIDEAN_NOTE_A");
    int velocityA = *apvts.getRawParameterValue("DUAL_EUCLIDEAN_VELOCITY_A");

    // Fetch params for machine B
    int stepsB = *apvts.getRawParameterValue("DUAL_EUCLIDEAN_STEPS_B");
    int pulsesB = *apvts.getRawParameterValue("DUAL_EUCLIDEAN_PULSES_B");
    int noteB = *apvts.getRawParameterValue("DUAL_EUCLIDEAN_NOTE_B");
    int velocityB = *apvts.getRawParameterValue("DUAL_EUCLIDEAN_VELOCITY_B");

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
                    float durationInBeats = rate * 0.9f;
                    int durationInSamples = static_cast<int>(durationInBeats * (60.0 / bpm) * sampleRate);
                    int samplePos = static_cast<int>(((lastBeat_ - currentBeat) * (60.0 / bpm)) * sampleRate);
                    if (samplePos < 0) samplePos = 0;
                    midiMessages.addEvent(juce::MidiMessage::noteOn(channel, noteA, (juce::uint8)velocityA), samplePos);
                    midiMessages.addEvent(juce::MidiMessage::noteOff(channel, noteA), samplePos + durationInSamples);
                }
            }

            // Machine B
            if (!patternB_.empty())
            {
                masterStepB_ = (masterStepB_ + 1) % patternB_.size();
                if (patternB_[masterStepB_])
                {
                    float durationInBeats = rate * 0.9f;
                    int durationInSamples = static_cast<int>(durationInBeats * (60.0 / bpm) * sampleRate);
                    int samplePos = static_cast<int>(((lastBeat_ - currentBeat) * (60.0 / bpm)) * sampleRate);
                    if (samplePos < 0) samplePos = 0;
                    midiMessages.addEvent(juce::MidiMessage::noteOn(channel, noteB, (juce::uint8)velocityB), samplePos);
                    midiMessages.addEvent(juce::MidiMessage::noteOff(channel, noteB), samplePos + durationInSamples);
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