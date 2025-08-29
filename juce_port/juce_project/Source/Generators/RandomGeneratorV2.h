#pragma once
#include "BaseGenerator.h"
#include <random>

class RandomGeneratorV2 : public BaseGenerator
{
public:
    RandomGeneratorV2();
    ~RandomGeneratorV2() override = default;

    juce::Array<PendingNoteOff> process(juce::MidiBuffer& midiMessages,
                                        juce::AudioProcessorValueTreeState& apvts,
                                        double sampleRate,
                                        double blockStartTime,
                                        double blockEndTime,
                                        int numSamples,
                                        juce::int64 totalSamples) override;

    void setScale(int rootNote, const std::vector<int>& scaleNotes) override;

    juce::MidiBuffer getPattern(double durationInBeats, juce::AudioProcessorValueTreeState& apvts, double sampleRate) override;
    void reset() override;

private:
    void generateEventsAt(double beat, juce::MidiBuffer& midiMessages, juce::AudioProcessorValueTreeState& apvts, double sampleRate, double blockStartTime);
    void addNote(juce::MidiBuffer& midiMessages, juce::AudioProcessorValueTreeState& apvts, double sampleRate, double beat, double blockStartTime);

    double lastBeat_ = -1.0;
    double nextEventBeat_ = 0.0;
    std::mt19937 randomEngine_;
    std::vector<int> scaleNotes_;
    int rootNote_ = 0;
};
