#pragma once
#include "BaseGenerator.h"

class RandomGeneratorV2 : public BaseGenerator
{
public:
    RandomGeneratorV2() = default;
    ~RandomGeneratorV2() override = default;

    void process(juce::MidiBuffer& midiMessages,
                 juce::AudioProcessorValueTreeState& apvts,
                 double sampleRate,
                 double currentBeat) override;
};
