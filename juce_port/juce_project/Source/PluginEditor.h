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

    // Looper GUI Components
    juce::TextButton recordButton;
    juce::TextButton playButton;
    juce::TextButton clearButton;
    juce::ComboBox looperModeSelector;
    juce::Slider pitchShiftSlider;
    juce::Slider playbackSpeedSlider;
    juce::ToggleButton reverseButton;
    juce::Label looperTitleLabel;
    juce::Label looperModeLabel;
    juce::Label pitchShiftLabel;
    juce::Label playbackSpeedLabel;
    juce::Label statusLabel;

    // Attachments for parameter automation
    std::unique_ptr<juce::SliderParameterAttachment> minNoteAttachment;
    std::unique_ptr<juce::SliderParameterAttachment> maxNoteAttachment;
    std::unique_ptr<juce::SliderParameterAttachment> probabilityAttachment;

    // Methods
    void setupSliders();
    void setupLabels();
    void setupScaleSelector();
    void setupLooperControls();

    // Button callbacks
    void recordButtonClicked();
    void playButtonClicked();
    void clearButtonClicked();
    void looperModeChanged();
    void updateStatusLabel();
    void updateButtonStates();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CreativeMidiGeneratorAudioProcessorEditor)
};