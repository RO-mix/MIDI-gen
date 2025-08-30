#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "TimelineComponent.h"

class LooperSectionComponent : public juce::Component,
                               public CreativeMidiGeneratorAudioProcessor::Listener
{
public:
    LooperSectionComponent(CreativeMidiGeneratorAudioProcessor&);
    ~LooperSectionComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // CreativeMidiGeneratorAudioProcessor::Listener overrides
    void playbackStateChanged(bool isPlaying) override;
    void looperStateChanged(bool isPlaying) override;

private:
    CreativeMidiGeneratorAudioProcessor& audioProcessor;

    // Row 1: Playback & Quantize
    juce::TextButton playButton;
    juce::ToggleButton throughToggle;
    juce::ToggleButton padModeToggle;
    juce::TextButton doubleButton;
    juce::TextButton splitButton;
    juce::TextButton quantizeButton;
    juce::ComboBox quantizeGridCombo;
    juce::TextButton variationButton;

    // Row 2: Capture
    juce::TextButton captureButton;
    juce::ComboBox captureDurationCombo;
    juce::ToggleButton captureOverdubToggle;
    juce::ComboBox recapturePeriodCombo;

    // Row 3: Recording
    juce::TextButton recordButton;
    juce::TextButton clearButton;
    juce::ComboBox recordLengthCombo;
    juce::ToggleButton recordOverdubToggle;
    juce::ComboBox actionQuantizeCombo;
    juce::TextButton saveButton;

    // Intensity Sliders
    juce::Slider bassIntensitySlider;
    juce::Slider midIntensitySlider;
    juce::Slider highIntensitySlider;

    TimelineComponent timelineComponent;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> throughAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> padModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> quantizeGridAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> captureDurationAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> captureOverdubAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> recapturePeriodAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> recordLengthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> recordOverdubAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> actionQuantizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bassIntensityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> midIntensityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> highIntensityAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LooperSectionComponent)
};
