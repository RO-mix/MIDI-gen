#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"

class DualEuclideanGeneratorPanel : public juce::Component
{
public:
    DualEuclideanGeneratorPanel(CreativeMidiGeneratorAudioProcessor&);
    ~DualEuclideanGeneratorPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    CreativeMidiGeneratorAudioProcessor& audioProcessor;

    // Machine A Components
    juce::Label machineALabel;
    juce::Slider stepsSliderA;
    juce::Slider pulsesSliderA;
    juce::Slider noteSliderA;
    juce::Slider velocitySliderA;
    juce::Slider durationBiasSliderA;
    juce::Slider deviationSliderA;
    juce::ToggleButton bipolarToggleA;

    // Machine B Components
    juce::Label machineBLabel;
    juce::Slider stepsSliderB;
    juce::Slider pulsesSliderB;
    juce::Slider noteSliderB;
    juce::Slider velocitySliderB;
    juce::Slider durationBiasSliderB;
    juce::Slider deviationSliderB;
    juce::ToggleButton bipolarToggleB;

    // Common Components
    juce::Slider noteProbabilitySlider;
    juce::ComboBox rateCombo;

    // Labels are omitted for brevity but would be included in a full implementation

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stepsAttachmentA;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pulsesAttachmentA;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> noteAttachmentA;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> velocityAttachmentA;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> durationBiasAttachmentA;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> deviationAttachmentA;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bipolarAttachmentA;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stepsAttachmentB;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pulsesAttachmentB;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> noteAttachmentB;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> velocityAttachmentB;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> durationBiasAttachmentB;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> deviationAttachmentB;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bipolarAttachmentB;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> noteProbabilityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rateAttachment;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DualEuclideanGeneratorPanel)
};
