#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/ToolbarComponent.h"
#include "UI/GeneratorSectionComponent.h"
#include "UI/LooperSectionComponent.h"

//==============================================================================
/**
*/
class CreativeMidiGeneratorAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    CreativeMidiGeneratorAudioProcessorEditor (CreativeMidiGeneratorAudioProcessor&);
    ~CreativeMidiGeneratorAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    CreativeMidiGeneratorAudioProcessor& audioProcessor;

    juce::LookAndFeel_V4 lookAndFeel;

    ToolbarComponent toolbarComponent;
    GeneratorSectionComponent generatorSectionComponent;
    LooperSectionComponent looperSectionComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CreativeMidiGeneratorAudioProcessorEditor)
};