#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"

class TimelineComponent : public juce::Component, public juce::Timer
{
public:
    TimelineComponent(CreativeMidiGeneratorAudioProcessor&);
    ~TimelineComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void timerCallback() override;

private:
    CreativeMidiGeneratorAudioProcessor& audioProcessor;

    // We can add methods here to get note data and playback progress
    // from the audioProcessor, which in turn gets it from the Looper.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelineComponent)
};
