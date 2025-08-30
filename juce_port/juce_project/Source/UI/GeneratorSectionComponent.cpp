#include "GeneratorSectionComponent.h"

GeneratorSectionComponent::GeneratorSectionComponent(CreativeMidiGeneratorAudioProcessor& p)
    : audioProcessor(p),
      randomPanel(p),
      euclideanPanel(p),
      dualEuclideanPanel(p),
      randomV2Panel(p)
{
    addAndMakeVisible(startButton);
    startButton.setButtonText("Start");
    startButton.setTooltip("Запускает или останавливает основной генератор MIDI.");
    startButton.onClick = [this] { audioProcessor.togglePlayback(); };

    addAndMakeVisible(generatorTypeCombo);
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(audioProcessor.apvts.getParameter("GENERATOR_TYPE")))
    {
        generatorTypeCombo.addItemList(choiceParam->choices, 1);
    }
    generatorTypeCombo.setTooltip("Выбор алгоритма, который будет использоваться для генерации MIDI.");
    generatorTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "GENERATOR_TYPE", generatorTypeCombo);

    addAndMakeVisible(randomPanel);
    addAndMakeVisible(euclideanPanel);
    addAndMakeVisible(dualEuclideanPanel);
    addAndMakeVisible(randomV2Panel);

    audioProcessor.apvts.addParameterListener("GENERATOR_TYPE", this);
    audioProcessor.addListener(this);

    // Set initial visibility and button state
    parameterChanged("GENERATOR_TYPE", audioProcessor.apvts.getRawParameterValue("GENERATOR_TYPE")->load());
    playbackStateChanged(audioProcessor.isPlaying());
}

GeneratorSectionComponent::~GeneratorSectionComponent()
{
    audioProcessor.removeListener(this);
    audioProcessor.apvts.removeParameterListener("GENERATOR_TYPE", this);
}

void GeneratorSectionComponent::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::darkgrey);
    auto bounds = getLocalBounds();
    g.drawVerticalLine(bounds.getX(), (float)bounds.getY(), (float)bounds.getBottom());
    g.drawVerticalLine(bounds.getRight() - 1, (float)bounds.getY(), (float)bounds.getBottom());
    g.drawHorizontalLine(bounds.getBottom() - 1, (float)bounds.getX(), (float)bounds.getRight());
    g.setFont(juce::FontOptions(18.0f));
    g.drawText("GENERATOR", getLocalBounds().removeFromTop(30), juce::Justification::centred, false);
}

void GeneratorSectionComponent::resized()
{
    auto bounds = getLocalBounds().reduced(15);
    bounds.removeFromTop(20); // spacing for title

    auto topRow = bounds.removeFromTop(30);
    startButton.setBounds(topRow.removeFromLeft(85));
    topRow.removeFromLeft(10);
    generatorTypeCombo.setBounds(topRow);

    bounds.removeFromTop(10);

    // All panels share the same bounds
    randomPanel.setBounds(bounds);
    euclideanPanel.setBounds(bounds);
    dualEuclideanPanel.setBounds(bounds);
    randomV2Panel.setBounds(bounds);
}

void GeneratorSectionComponent::playbackStateChanged(bool isPlaying)
{
    startButton.setButtonText(isPlaying ? "Stop" : "Start");
}

void GeneratorSectionComponent::looperStateChanged(bool isPlaying)
{
    // This component doesn't need to know about the looper's state.
    juce::ignoreUnused(isPlaying);
}

void GeneratorSectionComponent::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "GENERATOR_TYPE")
    {
        int choice = static_cast<int>(newValue);
        randomPanel.setVisible(choice == 0);
        euclideanPanel.setVisible(choice == 1);
        dualEuclideanPanel.setVisible(choice == 2);
        randomV2Panel.setVisible(choice == 3);
    }
}
