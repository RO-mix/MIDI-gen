#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "RandomGeneratorPanel.h"
#include "EuclideanGeneratorPanel.h"
#include "DualEuclideanGeneratorPanel.h"
#include "RandomGeneratorV2Panel.h"

class GeneratorSectionComponent : public juce::Component,
                                  public juce::AudioProcessorValueTreeState::Listener
{
public:
    GeneratorSectionComponent(CreativeMidiGeneratorAudioProcessor&);
    ~GeneratorSectionComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void parameterChanged(const juce::String& parameterID, float newValue) override;


private:
    CreativeMidiGeneratorAudioProcessor& audioProcessor;

    juce::ComboBox generatorTypeCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> generatorTypeAttachment;

    RandomGeneratorPanel randomPanel;
    EuclideanGeneratorPanel euclideanPanel;
    DualEuclideanGeneratorPanel dualEuclideanPanel;
    RandomGeneratorV2Panel randomV2Panel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GeneratorSectionComponent)
};
