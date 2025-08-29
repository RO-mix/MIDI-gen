#pragma once
#include "BaseGenerator.h"

class EuclideanGenerator : public BaseGenerator
{
public:
    EuclideanGenerator();
    ~EuclideanGenerator() override = default;

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
    // Internal state
    int currentStep_ = -1;
    double lastBeat_ = -1.0;
    int lastDeviation_ = 0;
    std::vector<bool> pattern_;
    std::vector<int> scaleNotes_;
    int rootNote_ = 0;

    // Helper methods
    void updatePattern(int steps, int pulses);
    int getDeviatedNote(int baseNote, int deviationRange, bool isBipolar);

    // Random number generation
    juce::Random random_;
};