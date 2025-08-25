#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"

class RandomGeneratorPanel : public juce::Component
{
public:
    RandomGeneratorPanel(CreativeMidiGeneratorAudioProcessor&);
    ~RandomGeneratorPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    CreativeMidiGeneratorAudioProcessor& audioProcessor;

    juce::Slider minNoteSlider;
    juce::Slider maxNoteSlider;
    juce::Slider maxVelocitySlider;
    juce::Slider velocityBiasSlider;
    juce::Slider noteProbabilitySlider;
    juce::Slider durationBiasSlider;
    juce::ComboBox rateCombo;
    juce::ToggleButton addCC74Toggle;

    juce::Label minNoteLabel;
    juce::Label maxNoteLabel;
    juce::Label maxVelocityLabel;
    juce::Label velocityBiasLabel;
    juce::Label noteProbabilityLabel;
    juce::Label durationBiasLabel;
    juce::Label rateLabel;
    juce::Label addCC74Label;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> minNoteAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> maxNoteAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> maxVelocityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> velocityBiasAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> noteProbabilityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> durationBiasAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> addCC74Attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RandomGeneratorPanel)
};
