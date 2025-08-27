#include "RandomGeneratorV2Panel.h"

RandomGeneratorV2Panel::RandomGeneratorV2Panel(CreativeMidiGeneratorAudioProcessor& p)
    : audioProcessor(p)
{
    addAndMakeVisible(minNoteLabel);
    minNoteLabel.setText("Min Note", juce::dontSendNotification);
    addAndMakeVisible(minNoteSlider);
    minNoteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "RANDOM_V2_MIN_NOTE", minNoteSlider);

    addAndMakeVisible(maxNoteLabel);
    maxNoteLabel.setText("Max Note", juce::dontSendNotification);
    addAndMakeVisible(maxNoteSlider);
    maxNoteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "RANDOM_V2_MAX_NOTE", maxNoteSlider);

    addAndMakeVisible(burstProbabilityLabel);
    burstProbabilityLabel.setText("Burst Probability", juce::dontSendNotification);
    addAndMakeVisible(burstProbabilitySlider);
    burstProbabilityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "RANDOM_V2_BURST_PROB", burstProbabilitySlider);

    addAndMakeVisible(noteProbabilityLabel);
    noteProbabilityLabel.setText("Note Probability", juce::dontSendNotification);
    addAndMakeVisible(noteProbabilitySlider);
    noteProbabilityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "RANDOM_V2_NOTE_PROB", noteProbabilitySlider);

    addAndMakeVisible(baseDurationLabel);
    baseDurationLabel.setText("Base Duration", juce::dontSendNotification);
    addAndMakeVisible(baseDurationCombo);
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(audioProcessor.apvts.getParameter("RANDOM_V2_BASE_DURATION")))
        baseDurationCombo.addItemList(choiceParam->choices, 1);
    baseDurationAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "RANDOM_V2_BASE_DURATION", baseDurationCombo);

    addAndMakeVisible(accelerationLabel);
    accelerationLabel.setText("Acceleration", juce::dontSendNotification);
    addAndMakeVisible(accelerationCombo);
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(audioProcessor.apvts.getParameter("RANDOM_V2_ACCELERATION")))
        accelerationCombo.addItemList(choiceParam->choices, 1);
    accelerationAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "RANDOM_V2_ACCELERATION", accelerationCombo);

    addAndMakeVisible(burstPatternLabel);
    burstPatternLabel.setText("Burst Pattern", juce::dontSendNotification);
    for (int i = 0; i < 8; ++i)
    {
        auto slider = std::make_unique<juce::Slider>();
        slider->setSliderStyle(juce::Slider::LinearBarVertical);
        addAndMakeVisible(*slider);
        juce::String paramId = "RANDOM_V2_BURST_PATTERN_" + juce::String(i);
        burstPatternAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, paramId, *slider));
        burstPatternSliders.push_back(std::move(slider));
    }
}

RandomGeneratorV2Panel::~RandomGeneratorV2Panel()
{
}

void RandomGeneratorV2Panel::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::darkgrey);
    g.drawRect(getLocalBounds(), 1);
}

void RandomGeneratorV2Panel::resized()
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
    createRow(burstProbabilityLabel, burstProbabilitySlider);
    createRow(noteProbabilityLabel, noteProbabilitySlider);

    auto comboRow = bounds.removeFromTop(rowHeight);
    baseDurationLabel.setBounds(comboRow.removeFromLeft(labelWidth));
    baseDurationCombo.setBounds(comboRow.removeFromLeft(150));
    accelerationLabel.setBounds(comboRow.removeFromLeft(labelWidth));
    accelerationCombo.setBounds(comboRow);
    bounds.removeFromTop(spacing);

    burstPatternLabel.setBounds(bounds.removeFromTop(20));
    auto patternArea = bounds.removeFromTop(60);
    int sliderWidth = patternArea.getWidth() / 8;
    for (const auto& slider : burstPatternSliders)
    {
        slider->setBounds(patternArea.removeFromLeft(sliderWidth));
    }
}
