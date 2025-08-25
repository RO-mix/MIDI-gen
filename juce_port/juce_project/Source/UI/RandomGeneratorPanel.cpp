#include "RandomGeneratorPanel.h"

RandomGeneratorPanel::RandomGeneratorPanel(CreativeMidiGeneratorAudioProcessor& p)
    : audioProcessor(p)
{
    // Min Note
    addAndMakeVisible(minNoteSlider);
    minNoteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "RANDOM_MIN_NOTE", minNoteSlider);
    addAndMakeVisible(minNoteLabel);
    minNoteLabel.setText("Min Note", juce::dontSendNotification);
    minNoteLabel.attachToComponent(&minNoteSlider, true);

    // Max Note
    addAndMakeVisible(maxNoteSlider);
    maxNoteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "RANDOM_MAX_NOTE", maxNoteSlider);
    addAndMakeVisible(maxNoteLabel);
    maxNoteLabel.setText("Max Note", juce::dontSendNotification);
    maxNoteLabel.attachToComponent(&maxNoteSlider, true);

    // Max Velocity
    addAndMakeVisible(maxVelocitySlider);
    maxVelocityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "RANDOM_MAX_VELOCITY", maxVelocitySlider);
    addAndMakeVisible(maxVelocityLabel);
    maxVelocityLabel.setText("Max Velocity", juce::dontSendNotification);
    maxVelocityLabel.attachToComponent(&maxVelocitySlider, true);

    // Velocity Bias
    addAndMakeVisible(velocityBiasSlider);
    velocityBiasAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "RANDOM_VELOCITY_BIAS", velocityBiasSlider);
    addAndMakeVisible(velocityBiasLabel);
    velocityBiasLabel.setText("Velocity Bias", juce::dontSendNotification);
    velocityBiasLabel.attachToComponent(&velocityBiasSlider, true);

    // Note Probability
    addAndMakeVisible(noteProbabilitySlider);
    noteProbabilityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "RANDOM_NOTE_PROBABILITY", noteProbabilitySlider);
    addAndMakeVisible(noteProbabilityLabel);
    noteProbabilityLabel.setText("Note Probability", juce::dontSendNotification);
    noteProbabilityLabel.attachToComponent(&noteProbabilitySlider, true);

    // Duration Bias
    addAndMakeVisible(durationBiasSlider);
    durationBiasAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "RANDOM_DURATION_BIAS", durationBiasSlider);
    addAndMakeVisible(durationBiasLabel);
    durationBiasLabel.setText("Duration Bias", juce::dontSendNotification);
    durationBiasLabel.attachToComponent(&durationBiasSlider, true);

    // Rate
    addAndMakeVisible(rateCombo);
    rateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "RANDOM_RATE", rateCombo);
    addAndMakeVisible(rateLabel);
    rateLabel.setText("Rate", juce::dontSendNotification);
    rateLabel.attachToComponent(&rateCombo, true);

    // Add CC74
    addAndMakeVisible(addCC74Toggle);
    addCC74Attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "RANDOM_ADD_CC74", addCC74Toggle);
    addAndMakeVisible(addCC74Label);
    addCC74Label.setText("Add CC74", juce::dontSendNotification);
    addCC74Label.attachToComponent(&addCC74Toggle, true);
}

RandomGeneratorPanel::~RandomGeneratorPanel()
{
}

void RandomGeneratorPanel::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::darkgrey);
    g.drawRect(getLocalBounds(), 1);
}

void RandomGeneratorPanel::resized()
{
    auto bounds = getLocalBounds().reduced(15);
    auto sliderHeight = 30;
    auto labelWidth = 100;
    auto spacing = 10;

    auto createRow = [&](juce::Component& comp) {
        auto row = bounds.removeFromTop(sliderHeight);
        comp.setBounds(row.withLeft(labelWidth));
        bounds.removeFromTop(spacing);
    };

    createRow(minNoteSlider);
    createRow(maxNoteSlider);
    createRow(maxVelocitySlider);
    createRow(velocityBiasSlider);
    createRow(noteProbabilitySlider);
    createRow(durationBiasSlider);
    createRow(rateCombo);
    createRow(addCC74Toggle);
}
