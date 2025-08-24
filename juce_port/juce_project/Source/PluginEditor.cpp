#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
CreativeMidiGeneratorAudioProcessorEditor::CreativeMidiGeneratorAudioProcessorEditor (CreativeMidiGeneratorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (500, 400);

    // Setup GUI components
    setupSliders();
    setupLabels();
    setupScaleSelector();
}

CreativeMidiGeneratorAudioProcessorEditor::~CreativeMidiGeneratorAudioProcessorEditor()
{
}

//==============================================================================
void CreativeMidiGeneratorAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // Draw a subtle border
    g.setColour (juce::Colours::grey.withAlpha(0.3f));
    g.drawRect (getLocalBounds(), 1);
}

void CreativeMidiGeneratorAudioProcessorEditor::resized()
{
    // Layout components
    auto area = getLocalBounds().reduced(10);
    auto titleHeight = 30;
    auto sliderHeight = 50;
    auto labelHeight = 20;

    // Title
    titleLabel.setBounds(area.removeFromTop(titleHeight));

    area.removeFromTop(10); // spacing

    // Min note controls
    minNoteLabel.setBounds(area.removeFromTop(labelHeight));
    minNoteSlider.setBounds(area.removeFromTop(sliderHeight));

    area.removeFromTop(10);

    // Max note controls
    maxNoteLabel.setBounds(area.removeFromTop(labelHeight));
    maxNoteSlider.setBounds(area.removeFromTop(sliderHeight));

    area.removeFromTop(10);

    // Probability controls
    probabilityLabel.setBounds(area.removeFromTop(labelHeight));
    probabilitySlider.setBounds(area.removeFromTop(sliderHeight));

    area.removeFromTop(10);

    // Scale controls
    scaleLabel.setBounds(area.removeFromTop(labelHeight));
    scaleSelector.setBounds(area.removeFromTop(sliderHeight));
}

void CreativeMidiGeneratorAudioProcessorEditor::setupSliders()
{
    // Min Note Slider
    addAndMakeVisible(minNoteSlider);
    minNoteSlider.setRange(0.0, 127.0, 1.0);
    minNoteSlider.setValue(60.0); // C4
    minNoteSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);

    // Max Note Slider
    addAndMakeVisible(maxNoteSlider);
    maxNoteSlider.setRange(0.0, 127.0, 1.0);
    maxNoteSlider.setValue(72.0); // C5
    maxNoteSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);

    // Probability Slider
    addAndMakeVisible(probabilitySlider);
    probabilitySlider.setRange(0.0, 1.0, 0.01);
    probabilitySlider.setValue(1.0);
    probabilitySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
}

void CreativeMidiGeneratorAudioProcessorEditor::setupLabels()
{
    // Title
    addAndMakeVisible(titleLabel);
    titleLabel.setText("Creative MIDI Generator", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);

    // Min Note Label
    addAndMakeVisible(minNoteLabel);
    minNoteLabel.setText("Min Note (MIDI):", juce::dontSendNotification);
    minNoteLabel.setJustificationType(juce::Justification::left);

    // Max Note Label
    addAndMakeVisible(maxNoteLabel);
    maxNoteLabel.setText("Max Note (MIDI):", juce::dontSendNotification);
    maxNoteLabel.setJustificationType(juce::Justification::left);

    // Probability Label
    addAndMakeVisible(probabilityLabel);
    probabilityLabel.setText("Note Probability:", juce::dontSendNotification);
    probabilityLabel.setJustificationType(juce::Justification::left);

    // Scale Label
    addAndMakeVisible(scaleLabel);
    scaleLabel.setText("Scale:", juce::dontSendNotification);
    scaleLabel.setJustificationType(juce::Justification::left);
}

void CreativeMidiGeneratorAudioProcessorEditor::setupScaleSelector()
{
    addAndMakeVisible(scaleSelector);
    scaleSelector.addItem("Chromatic", 1);
    scaleSelector.addItem("Major", 2);
    scaleSelector.addItem("Minor", 3);
    scaleSelector.addItem("Dorian", 4);
    scaleSelector.addItem("Mixolydian", 5);
    scaleSelector.setSelectedId(2); // Major by default
}