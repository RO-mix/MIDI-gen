#include "RandomGeneratorV2Panel.h"

RandomGeneratorV2Panel::RandomGeneratorV2Panel(CreativeMidiGeneratorAudioProcessor& p)
    : audioProcessor(p)
{
    // Note Selection
    addAndMakeVisible(minNoteSlider);
    minNoteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "RANDOM_V2_MIN_NOTE", minNoteSlider);
    addAndMakeVisible(maxNoteSlider);
    maxNoteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "RANDOM_V2_MAX_NOTE", maxNoteSlider);

    // Ambient Burst Engine
    addAndMakeVisible(burstProbabilitySlider);
    burstProbabilityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "RANDOM_V2_BURST_PROB", burstProbabilitySlider);
    addAndMakeVisible(noteProbabilitySlider);
    noteProbabilityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "RANDOM_V2_NOTE_PROB", noteProbabilitySlider);
    addAndMakeVisible(baseDurationCombo);
    baseDurationAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "RANDOM_V2_BASE_DURATION", baseDurationCombo);
    addAndMakeVisible(accelerationCombo);
    accelerationAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "RANDOM_V2_ACCELERATION", accelerationCombo);

    // Burst Pattern
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
    auto spacing = 10;

    // Note Selection
    bounds.removeFromTop(rowHeight); // Section Title
    minNoteSlider.setBounds(bounds.removeFromTop(rowHeight));
    maxNoteSlider.setBounds(bounds.removeFromTop(rowHeight));
    bounds.removeFromTop(spacing);

    // Ambient Burst Engine
    bounds.removeFromTop(rowHeight); // Section Title
    burstProbabilitySlider.setBounds(bounds.removeFromTop(rowHeight));
    noteProbabilitySlider.setBounds(bounds.removeFromTop(rowHeight));

    auto comboRow = bounds.removeFromTop(rowHeight);
    baseDurationCombo.setBounds(comboRow.removeFromLeft(comboRow.getWidth() / 2 - 5));
    accelerationCombo.setBounds(comboRow.removeFromRight(comboRow.getWidth() / 2 - 5));
    bounds.removeFromTop(spacing);

    // Burst Pattern
    bounds.removeFromTop(rowHeight); // Section Title
    auto patternArea = bounds;
    int sliderWidth = patternArea.getWidth() / 8;
    for (const auto& slider : burstPatternSliders)
    {
        slider->setBounds(patternArea.removeFromLeft(sliderWidth));
    }
}
