#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"

class RandomGeneratorV2Panel : public juce::Component
{
public:
    RandomGeneratorV2Panel(CreativeMidiGeneratorAudioProcessor&);
    ~RandomGeneratorV2Panel() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    CreativeMidiGeneratorAudioProcessor& audioProcessor;

    // Note Selection
    juce::Slider minNoteSlider;
    juce::Slider maxNoteSlider;

    // Ambient Burst Engine
    juce::Slider burstProbabilitySlider;
    juce::Slider noteProbabilitySlider;
    juce::ComboBox baseDurationCombo;
    juce::ComboBox accelerationCombo;

    // Burst Pattern
    std::vector<std::unique_ptr<juce::Slider>> burstPatternSliders;

    // Labels are omitted for brevity

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> minNoteAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> maxNoteAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> burstProbabilityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> noteProbabilityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> baseDurationAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> accelerationAttachment;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> burstPatternAttachments;

    juce::Label minNoteLabel;
    juce::Label maxNoteLabel;
    juce::Label burstProbabilityLabel;
    juce::Label noteProbabilityLabel;
    juce::Label baseDurationLabel;
    juce::Label accelerationLabel;
    juce::Label burstPatternLabel;
    juce::Label noteSelectionLabel;
    juce::Label engineLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RandomGeneratorV2Panel)
};
