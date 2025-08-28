#include "LooperSectionComponent.h"

LooperSectionComponent::LooperSectionComponent(CreativeMidiGeneratorAudioProcessor& p)
    : audioProcessor(p), timelineComponent(p)
{
    // Row 1
    addAndMakeVisible(playButton);
    playButton.setButtonText("Play");
    playButton.onClick = [this] { audioProcessor.toggleLooperPlay(); };

    addAndMakeVisible(throughToggle);
    throughToggle.setButtonText("THR");
    throughAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "LOOPER_THROUGH", throughToggle);

    addAndMakeVisible(padModeToggle);
    padModeToggle.setButtonText("PAD");
    padModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "LOOPER_PAD_MODE", padModeToggle);

    addAndMakeVisible(doubleButton);
    doubleButton.setButtonText("x2");
    doubleButton.onClick = [this] { audioProcessor.doubleLoop(); };

    addAndMakeVisible(splitButton);
    splitButton.setButtonText("/2");
    splitButton.onClick = [this] { audioProcessor.splitLoop(); };

    addAndMakeVisible(quantizeButton);
    quantizeButton.setButtonText("Quantize");
    quantizeButton.onClick = [this] { audioProcessor.quantizeLooper(); };

    addAndMakeVisible(quantizeGridCombo);
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(audioProcessor.apvts.getParameter("LOOPER_QUANTIZE_GRID")))
    {
        quantizeGridCombo.addItemList(choiceParam->choices, 1);
    }
    quantizeGridAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "LOOPER_QUANTIZE_GRID", quantizeGridCombo);

    addAndMakeVisible(variationButton);
    variationButton.setButtonText("Variation");
    variationButton.onClick = [this] { audioProcessor.generateLooperVariation(); };

    // Row 2
    addAndMakeVisible(captureButton);
    captureButton.setButtonText("Capture");
    captureButton.onClick = [this] { audioProcessor.captureFromGenerator(); };

    addAndMakeVisible(captureDurationCombo);
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(audioProcessor.apvts.getParameter("LOOPER_CAPTURE_DURATION")))
    {
        captureDurationCombo.addItemList(choiceParam->choices, 1);
    }
    captureDurationAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "LOOPER_CAPTURE_DURATION", captureDurationCombo);

    addAndMakeVisible(captureOverdubToggle);
    captureOverdubToggle.setButtonText("Overdub");
    captureOverdubAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "LOOPER_CAPTURE_OVERDUB", captureOverdubToggle);

    addAndMakeVisible(autoLabel);
    autoLabel.setText("AUTO", juce::dontSendNotification);
    autoLabel.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(recapturePeriodCombo);
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(audioProcessor.apvts.getParameter("LOOPER_RECAPTURE_PERIOD")))
    {
        recapturePeriodCombo.addItemList(choiceParam->choices, 1);
    }
    recapturePeriodAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "LOOPER_RECAPTURE_PERIOD", recapturePeriodCombo);

    // Row 3
    addAndMakeVisible(recordButton);
    recordButton.setButtonText("Record");
    recordButton.onClick = [this] { audioProcessor.toggleLooperRecord(); };

    addAndMakeVisible(recordLengthCombo);
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(audioProcessor.apvts.getParameter("LOOPER_RECORD_LENGTH")))
    {
        recordLengthCombo.addItemList(choiceParam->choices, 1);
    }
    recordLengthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "LOOPER_RECORD_LENGTH", recordLengthCombo);

    addAndMakeVisible(recordOverdubToggle);
    recordOverdubToggle.setButtonText("Overdub");
    recordOverdubAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "LOOPER_RECORD_OVERDUB", recordOverdubToggle);

    addAndMakeVisible(actionQuantizeCombo);
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(audioProcessor.apvts.getParameter("LOOPER_ACTION_QUANTIZE")))
    {
        actionQuantizeCombo.addItemList(choiceParam->choices, 1);
    }
    actionQuantizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "LOOPER_ACTION_QUANTIZE", actionQuantizeCombo);

    addAndMakeVisible(saveButton);
    saveButton.setButtonText("Save");
    // TODO: saveButton onClick needs to open a file chooser.

    // Intensity Sliders
    addAndMakeVisible(bassIntensitySlider);
    bassIntensityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "LOOPER_INTENSITY_BASS", bassIntensitySlider);
    addAndMakeVisible(midIntensitySlider);
    midIntensityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "LOOPER_INTENSITY_MID", midIntensitySlider);
    addAndMakeVisible(highIntensitySlider);
    highIntensityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "LOOPER_INTENSITY_HIGH", highIntensitySlider);

    addAndMakeVisible(timelineComponent);

    audioProcessor.addListener(this);
}

LooperSectionComponent::~LooperSectionComponent()
{
    audioProcessor.removeListener(this);
}

void LooperSectionComponent::playbackStateChanged(bool isPlaying)
{
    // The looper play button is not affected by the main generator's state.
    juce::ignoreUnused(isPlaying);
}

void LooperSectionComponent::looperStateChanged(bool isPlaying)
{
    playButton.setButtonText(isPlaying ? "Stop" : "Play");
    juce::Logger::writeToLog("UI: LooperSectionComponent received looper state change. isPlaying: " + juce::String(isPlaying ? "true" : "false"));
}

void LooperSectionComponent::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::darkgrey);
    g.drawRect(getLocalBounds(), 1);
    g.setFont(18.0f);
    g.drawText("LOOPER", getLocalBounds().removeFromTop(30), juce::Justification::centred, false);
}

void LooperSectionComponent::resized()
{
    auto bounds = getLocalBounds().reduced(15);
    bounds.removeFromTop(20); // Title spacing

    auto rowHeight = 30;
    auto spacing = 5;

    // Row 1
    auto row1 = bounds.removeFromTop(rowHeight);
    playButton.setBounds(row1.removeFromLeft(85));
    throughToggle.setBounds(row1.removeFromLeft(50));
    padModeToggle.setBounds(row1.removeFromLeft(50));
    doubleButton.setBounds(row1.removeFromLeft(40));
    splitButton.setBounds(row1.removeFromLeft(40));
    quantizeButton.setBounds(row1.removeFromLeft(85));
    quantizeGridCombo.setBounds(row1.removeFromLeft(80));
    variationButton.setBounds(row1.removeFromLeft(85));
    bounds.removeFromTop(spacing);

    // Row 2
    auto row2 = bounds.removeFromTop(rowHeight);
    captureButton.setBounds(row2.removeFromLeft(85));
    captureDurationCombo.setBounds(row2.removeFromLeft(130));
    captureOverdubToggle.setBounds(row2.removeFromLeft(80));
    autoLabel.setBounds(row2.removeFromLeft(50));
    recapturePeriodCombo.setBounds(row2.removeFromLeft(130));
    bounds.removeFromTop(spacing);

    // Row 3
    auto row3 = bounds.removeFromTop(rowHeight);
    recordButton.setBounds(row3.removeFromLeft(85));
    recordLengthCombo.setBounds(row3.removeFromLeft(130));
    recordOverdubToggle.setBounds(row3.removeFromLeft(80));
    actionQuantizeCombo.setBounds(row3.removeFromLeft(130));
    saveButton.setBounds(row3.removeFromLeft(85));
    bounds.removeFromTop(spacing);

    // Intensity Sliders
    auto sliderRow = bounds.removeFromTop(rowHeight);
    bassIntensitySlider.setBounds(sliderRow.removeFromLeft(sliderRow.getWidth() / 3));
    midIntensitySlider.setBounds(sliderRow.removeFromLeft(sliderRow.getWidth() / 2));
    highIntensitySlider.setBounds(sliderRow);
    bounds.removeFromTop(spacing);

    timelineComponent.setBounds(bounds);
}
