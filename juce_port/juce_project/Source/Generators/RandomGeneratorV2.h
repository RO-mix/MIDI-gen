#pragma once
#include "BaseGenerator.h"

class RandomGeneratorV2 : public BaseGenerator
{
public:
    RandomGeneratorV2();
    ~RandomGeneratorV2() override = default;

    void process(juce::MidiBuffer& midiMessages,
                 juce::AudioProcessorValueTreeState& apvts,
                 double sampleRate,
                 double currentBeat) override;

    void setScale(int rootNote, const std::vector<int>& scaleNotes) override;

private:
    void addNote(juce::MidiBuffer& midiMessages, juce::AudioProcessorValueTreeState& apvts, double sampleRate, double beat);

    double lastBeat_ = -1.0;
    double nextEventBeat_ = 0.0;
    std::mt19937 randomEngine_;
};
