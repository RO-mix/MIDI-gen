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

    juce::MidiBuffer getPattern(double durationInBeats, juce::AudioProcessorValueTreeState& apvts, double sampleRate) override;

private:
    // Internal state
    int masterStepA_ = -1;
    int masterStepB_ = -1;
    double lastBeat_ = -1.0;
    int lastDeviationA_ = 0;
    int lastDeviationB_ = 0;
    std::vector<bool> patternA_;
    std::vector<bool> patternB_;
    std::vector<int> scaleNotes_;
    int rootNote_ = 0;

    // Helper methods
    void updatePattern(std::vector<bool>& pattern, int steps, int pulses);
    int getDeviatedNote(int baseNote, int deviationRange, bool isBipolar, int& lastDeviation);

    // Random number generation
    juce::Random random_;
};