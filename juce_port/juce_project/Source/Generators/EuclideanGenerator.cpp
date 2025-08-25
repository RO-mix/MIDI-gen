#include "EuclideanGenerator.h"

EuclideanGenerator::EuclideanGenerator()
{
    random_.setSeed(juce::Time::currentTimeMillis());
    updatePattern(16, 4); // Default values
}

void EuclideanGenerator::process(juce::MidiBuffer& midiMessages,
                               juce::AudioProcessorValueTreeState& apvts,
                               double sampleRate,
                               double currentBeat)
{
    // Fetch parameters
    int steps = *apvts.getRawParameterValue("EUCLIDEAN_STEPS");
    int pulses = *apvts.getRawParameterValue("EUCLIDEAN_PULSES");
    int note = *apvts.getRawParameterValue("EUCLIDEAN_NOTE");
    int velocity = *apvts.getRawParameterValue("EUCLIDEAN_VELOCITY");
    int deviationRange = *apvts.getRawParameterValue("EUCLIDEAN_DEVIATION_RANGE");
    bool isBipolar = *apvts.getRawParameterValue("EUCLIDEAN_DEVIATION_BIPOLAR") > 0.5f;
    int rateChoice = *apvts.getRawParameterValue("EUCLIDEAN_RATE");
    float noteProbability = *apvts.getRawParameterValue("EUCLIDEAN_NOTE_PROBABILITY");
    int channel = *apvts.getRawParameterValue("MIDI_CHANNEL");
    double bpm = *apvts.getRawParameterValue("BPM");

    // Update pattern if needed
    if (steps != pattern_.size() || pulses != std::count(pattern_.begin(), pattern_.end(), true))
    {
        updatePattern(steps, pulses);
    }
    if (pattern_.empty()) return;


    // Map rateChoice to actual beat values
    float rateMap[] = { 16.0f, 8.0f, 4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 0.125f, 0.0625f };
    float rate = (rateChoice >= 0 && rateChoice < std::size(rateMap)) ? rateMap[rateChoice] : 0.25f;

    if (lastBeat_ < 0) lastBeat_ = currentBeat;

    while (lastBeat_ < currentBeat)
    {
        currentStep_ = (currentStep_ + 1) % steps;

        if (pattern_[currentStep_] && random_.nextFloat() < noteProbability)
        {
            int generatedNote = note;
            if (deviationRange > 0 && !scaleNotes_.empty())
            {
                int deviation = random_.nextInt(deviationRange + 1);
                if (isBipolar && random_.nextBool())
                {
                    deviation = -deviation;
                }

                auto it = std::find_if(scaleNotes_.begin(), scaleNotes_.end(), [&](int scaleNote){ return (note % 12) == (rootNote_ + scaleNote) % 12; });
                if(it != scaleNotes_.end())
                {
                    int baseIndex = std::distance(scaleNotes_.begin(), it);
                    int newIndex = (baseIndex + deviation);

                    // Wrap index within scale size
                    while(newIndex < 0) newIndex += scaleNotes_.size();
                    newIndex %= scaleNotes_.size();

                    int octave = note / 12;
                    generatedNote = (octave * 12) + rootNote_ + scaleNotes_[newIndex];
                }
            }

            generatedNote = juce::jlimit(0, 127, generatedNote);
            float durationInBeats = rate * 0.9f;
            int durationInSamples = static_cast<int>(durationInBeats * (60.0 / bpm) * sampleRate);

            int samplePos = static_cast<int>(((lastBeat_ - currentBeat) * (60.0 / bpm)) * sampleRate);
            if (samplePos < 0) samplePos = 0;

            midiMessages.addEvent(juce::MidiMessage::noteOn(channel, generatedNote, (juce::uint8)velocity), samplePos);
            midiMessages.addEvent(juce::MidiMessage::noteOff(channel, generatedNote), samplePos + durationInSamples);
        }

        lastBeat_ += rate;
    }
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