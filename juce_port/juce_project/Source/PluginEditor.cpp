#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
CreativeMidiGeneratorAudioProcessorEditor::CreativeMidiGeneratorAudioProcessorEditor (CreativeMidiGeneratorAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      toolbarComponent(p),
      generatorSectionComponent(p),
      looperSectionComponent(p)
{
    // Set the size for the new UI. This might be adjusted later.
    setSize (680, 800); // Increased height for looper

    addAndMakeVisible(toolbarComponent);
    addAndMakeVisible(generatorSectionComponent);
    addAndMakeVisible(looperSectionComponent);
}

CreativeMidiGeneratorAudioProcessorEditor::~CreativeMidiGeneratorAudioProcessorEditor()
{
}

//==============================================================================
void CreativeMidiGeneratorAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void CreativeMidiGeneratorAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    toolbarComponent.setBounds(bounds.removeFromTop(100));
    generatorSectionComponent.setBounds(bounds.removeFromTop(300));
    looperSectionComponent.setBounds(bounds);
}