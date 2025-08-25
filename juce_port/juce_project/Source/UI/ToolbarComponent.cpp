#include "ToolbarComponent.h"

ToolbarComponent::ToolbarComponent(CreativeMidiGeneratorAudioProcessor& p)
    : audioProcessor(p)
{
    // === Row 1: Playback and MIDI ===
    addAndMakeVisible(startButton);
    startButton.setButtonText("Start");
    startButton.onClick = [this] {
        audioProcessor.togglePlayback();
        bool isPlaying = audioProcessor.isPlaying();
        startButton.setButtonText(isPlaying ? "Stop" : "Start");
    };

    addAndMakeVisible(bpmSlider);
    bpmSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    bpmSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    bpmAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "BPM", bpmSlider);

    addAndMakeVisible(bpmLabel);
    bpmLabel.setText("BPM", juce::dontSendNotification);
    bpmLabel.attachToComponent(&bpmSlider, true);

    addAndMakeVisible(midiPortCombo);
    // TODO: MIDI port listing and selection needs to be implemented.
    // This is more complex than a simple parameter attachment.
    midiPortCombo.addItem("Default MIDI Out", 1);

    addAndMakeVisible(midiChannelSlider);
    midiChannelSlider.setSliderStyle(juce::Slider::IncDecButtons);
    midiChannelSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
    midiChannelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "MIDI_CHANNEL", midiChannelSlider);

    addAndMakeVisible(midiChannelLabel);
    midiChannelLabel.setText("Channel", juce::dontSendNotification);
    midiChannelLabel.attachToComponent(&midiChannelSlider, true);


    // === Row 2: Musical Context ===
    addAndMakeVisible(rootNoteCombo);
    rootNoteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "ROOT_NOTE", rootNoteCombo);

    addAndMakeVisible(rootNoteLabel);
    rootNoteLabel.setText("Root Note", juce::dontSendNotification);
    rootNoteLabel.attachToComponent(&rootNoteCombo, true);

    addAndMakeVisible(scaleCombo);
    scaleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "SCALE", scaleCombo);

    addAndMakeVisible(scaleLabel);
    scaleLabel.setText("Scale", juce::dontSendNotification);
    scaleLabel.attachToComponent(&scaleCombo, true);
}

ToolbarComponent::~ToolbarComponent()
{
}

void ToolbarComponent::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::darkgrey);
    g.drawRect(getLocalBounds(), 1);
}

void ToolbarComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    auto row1 = bounds.removeFromTop(40);
    auto row2 = bounds.removeFromTop(40);

    // Layout Row 1
    startButton.setBounds(row1.removeFromLeft(80));
    row1.removeFromLeft(10); // spacing
    midiPortCombo.setBounds(row1.removeFromRight(150));
    row1.removeFromRight(10);
    midiChannelSlider.setBounds(row1.removeFromRight(120));
    row1.removeFromRight(10);
    bpmSlider.setBounds(row1);

    // Layout Row 2
    rootNoteCombo.setBounds(row2.removeFromLeft(150));
    row2.removeFromLeft(10);
    scaleCombo.setBounds(row2.removeFromLeft(200));
}
