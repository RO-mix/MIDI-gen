#include "DualEuclideanGeneratorPanel.h"

DualEuclideanGeneratorPanel::DualEuclideanGeneratorPanel(CreativeMidiGeneratorAudioProcessor& p)
    : audioProcessor(p)
{
    // Machine A
    addAndMakeVisible(machineALabel);
    machineALabel.setText("Machine A", juce::dontSendNotification);
    machineALabel.setFont(juce::Font(juce::FontOptions(16.0f)));

    addAndMakeVisible(stepsSliderA);
    stepsAttachmentA = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_STEPS_A", stepsSliderA);
    addAndMakeVisible(stepsLabelA);
    stepsLabelA.setText("Steps A", juce::dontSendNotification);
    stepsLabelA.attachToComponent(&stepsSliderA, true);

    addAndMakeVisible(pulsesSliderA);
    pulsesAttachmentA = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_PULSES_A", pulsesSliderA);
    addAndMakeVisible(pulsesLabelA);
    pulsesLabelA.setText("Pulses A", juce::dontSendNotification);
    pulsesLabelA.attachToComponent(&pulsesSliderA, true);

    addAndMakeVisible(noteSliderA);
    noteAttachmentA = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_NOTE_A", noteSliderA);
    addAndMakeVisible(noteLabelA);
    noteLabelA.setText("Note A", juce::dontSendNotification);
    noteLabelA.attachToComponent(&noteSliderA, true);

    addAndMakeVisible(velocitySliderA);
    velocityAttachmentA = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_VELOCITY_A", velocitySliderA);
    addAndMakeVisible(velocityLabelA);
    velocityLabelA.setText("Velocity A", juce::dontSendNotification);
    velocityLabelA.attachToComponent(&velocitySliderA, true);

    addAndMakeVisible(durationBiasSliderA);
    durationBiasAttachmentA = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_DURATION_BIAS_A", durationBiasSliderA);
    addAndMakeVisible(durationBiasLabelA);
    durationBiasLabelA.setText("Duration A", juce::dontSendNotification);
    durationBiasLabelA.attachToComponent(&durationBiasSliderA, true);

    addAndMakeVisible(deviationSliderA);
    deviationAttachmentA = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_DEVIATION_A", deviationSliderA);
    addAndMakeVisible(deviationLabelA);
    deviationLabelA.setText("Deviation A", juce::dontSendNotification);
    deviationLabelA.attachToComponent(&deviationSliderA, true);

    addAndMakeVisible(bipolarToggleA);
    bipolarAttachmentA = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_BIPOLAR_A", bipolarToggleA);
    addAndMakeVisible(bipolarLabelA);
    bipolarLabelA.setText("Bipolar A", juce::dontSendNotification);
    bipolarLabelA.attachToComponent(&bipolarToggleA, true);

    // Machine B
    addAndMakeVisible(machineBLabel);
    machineBLabel.setText("Machine B", juce::dontSendNotification);
    machineBLabel.setFont(juce::Font(juce::FontOptions(16.0f)));

    addAndMakeVisible(stepsSliderB);
    stepsAttachmentB = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_STEPS_B", stepsSliderB);
    addAndMakeVisible(stepsLabelB);
    stepsLabelB.setText("Steps B", juce::dontSendNotification);
    stepsLabelB.attachToComponent(&stepsSliderB, true);

    addAndMakeVisible(pulsesSliderB);
    pulsesAttachmentB = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_PULSES_B", pulsesSliderB);
    addAndMakeVisible(pulsesLabelB);
    pulsesLabelB.setText("Pulses B", juce::dontSendNotification);
    pulsesLabelB.attachToComponent(&pulsesSliderB, true);

    addAndMakeVisible(noteSliderB);
    noteAttachmentB = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_NOTE_B", noteSliderB);
    addAndMakeVisible(noteLabelB);
    noteLabelB.setText("Note B", juce::dontSendNotification);
    noteLabelB.attachToComponent(&noteSliderB, true);

    addAndMakeVisible(velocitySliderB);
    velocityAttachmentB = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_VELOCITY_B", velocitySliderB);
    addAndMakeVisible(velocityLabelB);
    velocityLabelB.setText("Velocity B", juce::dontSendNotification);
    velocityLabelB.attachToComponent(&velocitySliderB, true);

    addAndMakeVisible(durationBiasSliderB);
    durationBiasAttachmentB = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_DURATION_BIAS_B", durationBiasSliderB);
    addAndMakeVisible(durationBiasLabelB);
    durationBiasLabelB.setText("Duration B", juce::dontSendNotification);
    durationBiasLabelB.attachToComponent(&durationBiasSliderB, true);

    addAndMakeVisible(deviationSliderB);
    deviationAttachmentB = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_DEVIATION_B", deviationSliderB);
    addAndMakeVisible(deviationLabelB);
    deviationLabelB.setText("Deviation B", juce::dontSendNotification);
    deviationLabelB.attachToComponent(&deviationSliderB, true);

    addAndMakeVisible(bipolarToggleB);
    bipolarAttachmentB = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_BIPOLAR_B", bipolarToggleB);
    addAndMakeVisible(bipolarLabelB);
    bipolarLabelB.setText("Bipolar B", juce::dontSendNotification);
    bipolarLabelB.attachToComponent(&bipolarToggleB, true);


    // Common
    addAndMakeVisible(noteProbabilitySlider);
    noteProbabilityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_NOTE_PROBABILITY", noteProbabilitySlider);
    addAndMakeVisible(noteProbabilityLabel);
    noteProbabilityLabel.setText("Note Probability", juce::dontSendNotification);
    noteProbabilityLabel.attachToComponent(&noteProbabilitySlider, true);

    addAndMakeVisible(rateCombo);
    rateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "DUAL_EUCLIDEAN_RATE", rateCombo);
    addAndMakeVisible(rateLabel);
    rateLabel.setText("Rate", juce::dontSendNotification);
    rateLabel.attachToComponent(&rateCombo, true);
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
    auto rowHeight = 24;
    auto labelWidth = 100;
    auto spacing = 4;
    auto halfWidth = bounds.getWidth() / 2 - 10;

    auto columnA = bounds.removeFromLeft(halfWidth);
    bounds.removeFromLeft(20);
    auto columnB = bounds;

    auto createRow = [&](juce::Component& comp, juce::Rectangle<int>& col) {
        auto row = col.removeFromTop(rowHeight);
        comp.setBounds(row.withTrimmedLeft(labelWidth));
        col.removeFromTop(spacing);
    };

    // Layout Column A
    machineALabel.setBounds(columnA.removeFromTop(rowHeight));
    columnA.removeFromTop(spacing);
    createRow(stepsSliderA, columnA);
    createRow(pulsesSliderA, columnA);
    createRow(noteSliderA, columnA);
    createRow(velocitySliderA, columnA);
    createRow(durationBiasSliderA, columnA);
    createRow(deviationSliderA, columnA);
    createRow(bipolarToggleA, columnA);

    // Layout Column B
    machineBLabel.setBounds(columnB.removeFromTop(rowHeight));
    columnB.removeFromTop(spacing);
    createRow(stepsSliderB, columnB);
    createRow(pulsesSliderB, columnB);
    createRow(noteSliderB, columnB);
    createRow(velocitySliderB, columnB);
    createRow(durationBiasSliderB, columnB);
    createRow(deviationSliderB, columnB);
    createRow(bipolarToggleB, columnB);

    // Layout Common Controls
    auto commonBounds = getLocalBounds().reduced(15).removeFromBottom(rowHeight * 2 + spacing);
    createRow(noteProbabilitySlider, commonBounds);
    createRow(rateCombo, commonBounds);
}
