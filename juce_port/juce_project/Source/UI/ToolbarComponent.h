#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"

class ToolbarComponent : public juce::Component
{
public:
    ToolbarComponent(CreativeMidiGeneratorAudioProcessor&);
    ~ToolbarComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    CreativeMidiGeneratorAudioProcessor& audioProcessor;

    // Row 1: Playback and MIDI
    juce::TextButton startButton;
    juce::Slider bpmSlider;
    juce::ComboBox midiPortCombo;
    juce::Slider midiChannelSlider;

    // Row 2: Musical Context
    juce::ComboBox rootNoteCombo;
    juce::ComboBox scaleCombo;

    // Labels
    juce::Label bpmLabel;
    juce::Label midiChannelLabel;
    juce::Label rootNoteLabel;
    juce::Label scaleLabel;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> startAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bpmAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> midiChannelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rootNoteAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> scaleAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToolbarComponent)
};
