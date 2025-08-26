#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"

class EuclideanGeneratorPanel : public juce::Component
{
public:
    EuclideanGeneratorPanel(CreativeMidiGeneratorAudioProcessor&);
    ~EuclideanGeneratorPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    CreativeMidiGeneratorAudioProcessor& audioProcessor;

    juce::Slider stepsSlider;
    juce::Slider pulsesSlider;
    juce::Slider noteSlider;
    juce::Slider velocitySlider;
    juce::Slider deviationRangeSlider;
    juce::ToggleButton deviationIsBipolarToggle;
    juce::ComboBox rateCombo;
    juce::Slider durationBiasSlider;
    juce::Slider noteProbabilitySlider;

    juce::Label stepsLabel;
    juce::Label pulsesLabel;
    juce::Label noteLabel;
    juce::Label velocityLabel;
    juce::Label deviationRangeLabel;
    juce::Label deviationIsBipolarLabel;
    juce::Label rateLabel;
    juce::Label durationBiasLabel;
    juce::Label noteProbabilityLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stepsAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pulsesAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> noteAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> velocityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> deviationRangeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> deviationIsBipolarAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> durationBiasAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> noteProbabilityAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EuclideanGeneratorPanel)
};
