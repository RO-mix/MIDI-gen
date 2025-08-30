#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"

class ToolbarComponent : public juce::Component,
                         public CreativeMidiGeneratorAudioProcessor::Listener
{
public:
    ToolbarComponent(CreativeMidiGeneratorAudioProcessor&);
    ~ToolbarComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // CreativeMidiGeneratorAudioProcessor::Listener overrides
    void playbackStateChanged(bool isPlaying) override;
    void looperStateChanged(bool isPlaying) override;

private:
    CreativeMidiGeneratorAudioProcessor& audioProcessor;

    // Row 1: Playback and MIDI
    juce::TextButton startButton;
    juce::Slider bpmSlider;
    juce::Slider midiChannelSlider;

    // Row 2: Musical Context
    juce::ComboBox rootNoteCombo;
    juce::ComboBox scaleCombo;
    juce::ComboBox presetCombo;
    juce::TextButton savePresetButton;

    // Preset Management
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::File presetDirectory;
    juce::StringArray presetFiles;
    void scanForPresets();
    void savePreset();
    void loadPreset(int presetIndex);

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
