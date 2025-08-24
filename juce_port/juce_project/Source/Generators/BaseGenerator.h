#pragma once
#include <JuceHeader.h>

class BaseGenerator
{
public:
    virtual ~BaseGenerator() = default;
    
    virtual std::pair<std::vector<std::pair<juce::MidiMessage, double>>, double> 
    generate(double currentBeat) = 0;
    
    virtual void setParameter(const juce::String& paramId, float value) = 0;
    virtual float getParameter(const juce::String& paramId) const = 0;
};