#pragma once
#include "BaseGenerator.h"

class DualEuclideanGenerator : public BaseGenerator
{
public:
    DualEuclideanGenerator();
    ~DualEuclideanGenerator() override = default;

    void process(juce::MidiBuffer& midiMessages,
                 juce::AudioProcessorValueTreeState& apvts,
                 double sampleRate,
                 double currentBeat) override;

    void setScale(int rootNote, const std::vector<int>& scaleNotes) override;

private:
    // Internal state
    int masterStepA_ = -1;
    int masterStepB_ = -1;
    double lastBeat_ = -1.0;
    std::vector<bool> patternA_;
    std::vector<bool> patternB_;
    std::vector<int> scaleNotes_;
    int rootNote_ = 0;

    // Helper methods
    void updatePattern(std::vector<bool>& pattern, int steps, int pulses);

    // Random number generation
    juce::Random random_;
};