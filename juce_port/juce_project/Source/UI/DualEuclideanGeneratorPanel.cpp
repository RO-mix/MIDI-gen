#include "DualEuclideanGeneratorPanel.h"

DualEuclideanGeneratorPanel::DualEuclideanGeneratorPanel(CreativeMidiGeneratorAudioProcessor& p)
    : audioProcessor(p)
{
    // Machine A
    addAndMakeVisible(machineALabel);
    machineALabel.setText("Machine A", juce::dontSendNotification);
    machineALabel.setFont(juce::Font(16.0f).withStyle(juce::Font::bold));

    addAndMakeVisible(stepsSliderA);
    stepsAttachmentA = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_STEPS_A", stepsSliderA);
    addAndMakeVisible(pulsesSliderA);
    pulsesAttachmentA = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_PULSES_A", pulsesSliderA);
    addAndMakeVisible(noteSliderA);
    noteAttachmentA = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_NOTE_A", noteSliderA);
    addAndMakeVisible(velocitySliderA);
    velocityAttachmentA = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_VELOCITY_A", velocitySliderA);
    addAndMakeVisible(durationBiasSliderA);
    durationBiasAttachmentA = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_DURATION_BIAS_A", durationBiasSliderA);
    addAndMakeVisible(deviationSliderA);
    deviationAttachmentA = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_DEVIATION_A", deviationSliderA);
    addAndMakeVisible(bipolarToggleA);
    bipolarAttachmentA = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_BIPOLAR_A", bipolarToggleA);

    // Machine B
    addAndMakeVisible(machineBLabel);
    machineBLabel.setText("Machine B", juce::dontSendNotification);
    machineBLabel.setFont(juce::Font(16.0f).withStyle(juce::Font::bold));

    addAndMakeVisible(stepsSliderB);
    stepsAttachmentB = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_STEPS_B", stepsSliderB);
    addAndMakeVisible(pulsesSliderB);
    pulsesAttachmentB = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_PULSES_B", pulsesSliderB);
    addAndMakeVisible(noteSliderB);
    noteAttachmentB = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_NOTE_B", noteSliderB);
    addAndMakeVisible(velocitySliderB);
    velocityAttachmentB = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_VELOCITY_B", velocitySliderB);
    addAndMakeVisible(durationBiasSliderB);
    durationBiasAttachmentB = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_DURATION_BIAS_B", durationBiasSliderB);
    addAndMakeVisible(deviationSliderB);
    deviationAttachmentB = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_DEVIATION_B", deviationSliderB);
    addAndMakeVisible(bipolarToggleB);
    bipolarAttachmentB = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_BIPOLAR_B", bipolarToggleB);

    // Common
    addAndMakeVisible(noteProbabilitySlider);
    noteProbabilityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_NOTE_PROBABILITY", noteProbabilitySlider);
    addAndMakeVisible(rateCombo);
    rateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_RATE", rateCombo);
}

DualEuclideanGeneratorPanel::~DualEuclideanGeneratorPanel()
{
}

void DualEuclideanGeneratorPanel::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::darkgrey);
    g.drawRect(getLocalBounds(), 1);
}

void DualEuclideanGeneratorPanel::resized()
{
    auto bounds = getLocalBounds().reduced(15);
    auto halfWidth = bounds.getWidth() / 2 - 10;

    auto columnA = bounds.removeFromLeft(halfWidth);
    bounds.removeFromLeft(20); // spacing
    auto columnB = bounds;

    auto commonRow = getLocalBounds().reduced(15).removeFromBottom(60);

    // Layout Column A
    machineALabel.setBounds(columnA.removeFromTop(20));
    stepsSliderA.setBounds(columnA.removeFromTop(30));
    pulsesSliderA.setBounds(columnA.removeFromTop(30));
    noteSliderA.setBounds(columnA.removeFromTop(30));
    velocitySliderA.setBounds(columnA.removeFromTop(30));
    durationBiasSliderA.setBounds(columnA.removeFromTop(30));
    deviationSliderA.setBounds(columnA.removeFromTop(30));
    bipolarToggleA.setBounds(columnA.removeFromTop(30));

    // Layout Column B
    machineBLabel.setBounds(columnB.removeFromTop(20));
    stepsSliderB.setBounds(columnB.removeFromTop(30));
    pulsesSliderB.setBounds(columnB.removeFromTop(30));
    noteSliderB.setBounds(columnB.removeFromTop(30));
    velocitySliderB.setBounds(columnB.removeFromTop(30));
    durationBiasSliderB.setBounds(columnB.removeFromTop(30));
    deviationSliderB.setBounds(columnB.removeFromTop(30));
    bipolarToggleB.setBounds(columnB.removeFromTop(30));

    // Layout Common
    noteProbabilitySlider.setBounds(commonRow.removeFromTop(30));
    rateCombo.setBounds(commonRow.removeFromTop(30));
}
