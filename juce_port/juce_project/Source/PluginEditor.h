#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

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

    // GUI Components
    juce::Slider minNoteSlider;
    juce::Slider maxNoteSlider;
    juce::Slider probabilitySlider;
    juce::ComboBox scaleSelector;
    juce::Label minNoteLabel;
    juce::Label maxNoteLabel;
    juce::Label probabilityLabel;
    juce::Label scaleLabel;
    juce::Label titleLabel;

    // Attachments for parameter automation
    std::unique_ptr<juce::SliderParameterAttachment> minNoteAttachment;
    std::unique_ptr<juce::SliderParameterAttachment> maxNoteAttachment;
    std::unique_ptr<juce::SliderParameterAttachment> probabilityAttachment;

    // Methods
    void setupSliders();
    void setupLabels();
    void setupScaleSelector();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CreativeMidiGeneratorAudioProcessorEditor)
};