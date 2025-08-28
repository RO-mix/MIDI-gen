#include "EuclideanGeneratorPanel.h"

EuclideanGeneratorPanel::EuclideanGeneratorPanel(CreativeMidiGeneratorAudioProcessor& p)
    : audioProcessor(p)
{
    addAndMakeVisible(stepsSlider);
    stepsAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "EUCLIDEAN_STEPS", stepsSlider);
    addAndMakeVisible(stepsLabel);
    stepsLabel.setText("Steps", juce::dontSendNotification);
    stepsLabel.attachToComponent(&stepsSlider, true);

    addAndMakeVisible(pulsesSlider);
    pulsesAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "EUCLIDEAN_PULSES", pulsesSlider);
    addAndMakeVisible(pulsesLabel);
    pulsesLabel.setText("Pulses", juce::dontSendNotification);
    pulsesLabel.attachToComponent(&pulsesSlider, true);

    addAndMakeVisible(noteSlider);
    noteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "EUCLIDEAN_NOTE", noteSlider);
    addAndMakeVisible(noteLabel);
    noteLabel.setText("Note", juce::dontSendNotification);
    noteLabel.attachToComponent(&noteSlider, true);

    addAndMakeVisible(velocitySlider);
    velocityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "EUCLIDEAN_VELOCITY", velocitySlider);
    addAndMakeVisible(velocityLabel);
    velocityLabel.setText("Velocity", juce::dontSendNotification);
    velocityLabel.attachToComponent(&velocitySlider, true);

    addAndMakeVisible(deviationRangeSlider);
    deviationRangeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "EUCLIDEAN_DEVIATION_RANGE", deviationRangeSlider);
    addAndMakeVisible(deviationRangeLabel);
    deviationRangeLabel.setText("Deviation Range", juce::dontSendNotification);
    deviationRangeLabel.attachToComponent(&deviationRangeSlider, true);

    addAndMakeVisible(deviationIsBipolarToggle);
    deviationIsBipolarAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "EUCLIDEAN_DEVIATION_BIPOLAR", deviationIsBipolarToggle);
    addAndMakeVisible(deviationIsBipolarLabel);
    deviationIsBipolarLabel.setText("Bipolar Deviation", juce::dontSendNotification);
    deviationIsBipolarLabel.attachToComponent(&deviationIsBipolarToggle, true);

    addAndMakeVisible(rateCombo);
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(audioProcessor.apvts.getParameter("EUCLIDEAN_RATE")))
    {
        rateCombo.addItemList(choiceParam->choices, 1);
    }
    rateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "EUCLIDEAN_RATE", rateCombo);
    addAndMakeVisible(rateLabel);
    rateLabel.setText("Rate", juce::dontSendNotification);
    rateLabel.attachToComponent(&rateCombo, true);

    addAndMakeVisible(durationBiasSlider);
    durationBiasAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "EUCLIDEAN_DURATION_BIAS", durationBiasSlider);
    addAndMakeVisible(durationBiasLabel);
    durationBiasLabel.setText("Duration Bias", juce::dontSendNotification);
    durationBiasLabel.attachToComponent(&durationBiasSlider, true);

    addAndMakeVisible(noteProbabilitySlider);
    noteProbabilityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "EUCLIDEAN_NOTE_PROBABILITY", noteProbabilitySlider);
    addAndMakeVisible(noteProbabilityLabel);
    noteProbabilityLabel.setText("Note Probability", juce::dontSendNotification);
    noteProbabilityLabel.attachToComponent(&noteProbabilitySlider, true);
}

EuclideanGeneratorPanel::~EuclideanGeneratorPanel()
{
}

void EuclideanGeneratorPanel::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::darkgrey);
    g.drawRect(getLocalBounds(), 1);
}

void EuclideanGeneratorPanel::resized()
{
    auto bounds = getLocalBounds().reduced(15);
    auto itemHeight = 30;
    auto spacing = 10;

    auto row = bounds.removeFromTop(itemHeight);
    auto halfWidth = row.getWidth() / 2;

    // Row 1
    stepsSlider.setBounds(row.removeFromLeft(halfWidth).withTrimmedLeft(120));
    pulsesSlider.setBounds(row.withTrimmedLeft(120));
    bounds.removeFromTop(spacing);

    // Row 2
    row = bounds.removeFromTop(itemHeight);
    noteSlider.setBounds(row.removeFromLeft(halfWidth).withTrimmedLeft(120));
    velocitySlider.setBounds(row.withTrimmedLeft(120));
    bounds.removeFromTop(spacing);

    // Row 3
    row = bounds.removeFromTop(itemHeight);
    deviationRangeSlider.setBounds(row.removeFromLeft(halfWidth).withTrimmedLeft(120));
    deviationIsBipolarToggle.setBounds(row.withTrimmedLeft(120));
    bounds.removeFromTop(spacing);

    // Row 4
    row = bounds.removeFromTop(itemHeight);
    rateCombo.setBounds(row.removeFromLeft(halfWidth).withTrimmedLeft(120));
    durationBiasSlider.setBounds(row.withTrimmedLeft(120));
    bounds.removeFromTop(spacing);

    // Row 5
    row = bounds.removeFromTop(itemHeight);
    noteProbabilitySlider.setBounds(row.removeFromLeft(halfWidth).withTrimmedLeft(120));
}
