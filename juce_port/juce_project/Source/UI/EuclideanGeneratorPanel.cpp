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
    auto sliderHeight = 30;
    auto labelWidth = 120;
    auto spacing = 10;

    auto createRow = [&](juce::Component& comp) {
        auto row = bounds.removeFromTop(sliderHeight);
        comp.setBounds(row.withLeft(labelWidth));
        bounds.removeFromTop(spacing);
    };

    createRow(stepsSlider);
    createRow(pulsesSlider);
    createRow(noteSlider);
    createRow(velocitySlider);
    createRow(deviationRangeSlider);
    createRow(durationBiasSlider);
    createRow(deviationIsBipolarToggle);
    createRow(rateCombo);
    createRow(noteProbabilitySlider);
}
