#pragma once
#include <JuceHeader.h>
#include "../Generators/BaseGenerator.h"

class Sequencer
{
public:
    Sequencer();
    ~Sequencer();
    
    void setGenerator(std::unique_ptr<BaseGenerator> gen);
    void setTempo(double bpm);
    void setPlaying(bool playing);
    
    void processBlock(juce::MidiBuffer& midiMessages, int numSamples);
    
private:
    std::unique_ptr<BaseGenerator> generator;
    double tempo = 120.0;
    bool isPlaying = false;
    double currentBeat = 0.0;
    double samplesPerBeat = 0.0;
    int sampleRate = 44100;
    
    void updateTiming();
};