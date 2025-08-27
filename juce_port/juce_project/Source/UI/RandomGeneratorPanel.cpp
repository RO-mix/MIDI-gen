#include "RandomGeneratorPanel.h"

RandomGeneratorPanel::RandomGeneratorPanel(CreativeMidiGeneratorAudioProcessor& p)
    : audioProcessor(p)
{
    // Min Note
    addAndMakeVisible(minNoteSlider);
    minNoteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "RANDOM_MIN_NOTE", minNoteSlider);
    addAndMakeVisible(minNoteLabel);
    minNoteLabel.setText("Min Note", juce::dontSendNotification);
    // Max Note
    addAndMakeVisible(maxNoteSlider);
    maxNoteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "RANDOM_MAX_NOTE", maxNoteSlider);
    addAndMakeVisible(maxNoteLabel);
    maxNoteLabel.setText("Max Note", juce::dontSendNotification);

    // Max Velocity
    addAndMakeVisible(maxVelocitySlider);
    maxVelocityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "RANDOM_MAX_VELOCITY", maxVelocitySlider);
    addAndMakeVisible(maxVelocityLabel);
    maxVelocityLabel.setText("Max Velocity", juce::dontSendNotification);

    // Velocity Bias
    addAndMakeVisible(velocityBiasSlider);
    velocityBiasAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "RANDOM_VELOCITY_BIAS", velocityBiasSlider);
    addAndMakeVisible(velocityBiasLabel);
    velocityBiasLabel.setText("Velocity Bias", juce::dontSendNotification);

    // Note Probability
    addAndMakeVisible(noteProbabilitySlider);
    noteProbabilityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "RANDOM_NOTE_PROBABILITY", noteProbabilitySlider);
    addAndMakeVisible(noteProbabilityLabel);
    noteProbabilityLabel.setText("Note Probability", juce::dontSendNotification);

    // Duration Bias
    addAndMakeVisible(durationBiasSlider);
    durationBiasAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "RANDOM_DURATION_BIAS", durationBiasSlider);
    addAndMakeVisible(durationBiasLabel);
    durationBiasLabel.setText("Duration Bias", juce::dontSendNotification);

    // Rate
    addAndMakeVisible(rateCombo);
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(audioProcessor.apvts.getParameter("RANDOM_RATE")))
    {
        rateCombo.addItemList(choiceParam->choices, 1);
    }
    rateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "RANDOM_RATE", rateCombo);
    addAndMakeVisible(rateLabel);
    rateLabel.setText("Rate", juce::dontSendNotification);

    // Add CC74
    addAndMakeVisible(addCC74Toggle);
    addCC74Toggle.setButtonText("");
    addCC74Toggle.setClickingTogglesState(true);
    addCC74Toggle.setLookAndFeel(&checkboxLookAndFeel);
    addCC74Attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "RANDOM_ADD_CC74", addCC74Toggle);
    addAndMakeVisible(addCC74Label);
    addCC74Label.setText("Add CC74", juce::dontSendNotification);
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
    auto rowHeight = 30;
    auto labelWidth = 120;
    auto spacing = 10;

    auto createRow = [&](juce::Label& label, juce::Component& comp) {
        auto row = bounds.removeFromTop(rowHeight);
        label.setBounds(row.removeFromLeft(labelWidth));
        comp.setBounds(row);
        bounds.removeFromTop(spacing);
    };

    createRow(minNoteLabel, minNoteSlider);
    createRow(maxNoteLabel, maxNoteSlider);
    createRow(maxVelocityLabel, maxVelocitySlider);
    createRow(velocityBiasLabel, velocityBiasSlider);
    createRow(noteProbabilityLabel, noteProbabilitySlider);
    createRow(durationBiasLabel, durationBiasSlider);
    createRow(rateLabel, rateCombo);
    createRow(addCC74Label, addCC74Toggle);
}
