#pragma once
#include "BaseGenerator.h"
#include <random>

class RandomGenerator : public BaseGenerator
{
public:
    RandomGenerator();
    ~RandomGenerator() override = default;

    void process(juce::MidiBuffer& midiMessages,
                 juce::AudioProcessorValueTreeState& apvts,
                 double sampleRate,
                 double currentBeat) override;

    void setScale(int rootNote, const std::vector<int>& scaleNotes) override;

private:
    // Internal state, not parameters
    double lastBeat_ = -1.0;
    std::mt19937 randomEngine_;
    std::uniform_real_distribution<float> distribution_;
    std::vector<int> scaleNotes_;
    int rootNote_ = 0;

    // Helper methods
    int calculateVelocity(float bias, int maxVelocity) const;
    float getRandomDuration(float bias) const;
};