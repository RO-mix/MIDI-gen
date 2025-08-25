#include "ToolbarComponent.h"

ToolbarComponent::ToolbarComponent(CreativeMidiGeneratorAudioProcessor& p)
    : audioProcessor(p)
{
    // === Row 1: Playback and MIDI ===
    addAndMakeVisible(startButton);
    startButton.setButtonText("Start");
    startButton.onClick = [this] {
        audioProcessor.togglePlayback();
        bool isPlaying = audioProcessor.isPlaying();
        startButton.setButtonText(isPlaying ? "Stop" : "Start");
    };

    addAndMakeVisible(bpmSlider);
    bpmSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    bpmSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    bpmAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "BPM", bpmSlider);

    addAndMakeVisible(bpmLabel);
    bpmLabel.setText("BPM", juce::dontSendNotification);
    bpmLabel.attachToComponent(&bpmSlider, true);

    addAndMakeVisible(midiChannelSlider);
    midiChannelSlider.setSliderStyle(juce::Slider::IncDecButtons);
    midiChannelSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
    midiChannelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "MIDI_CHANNEL", midiChannelSlider);

    addAndMakeVisible(midiChannelLabel);
    midiChannelLabel.setText("MIDI CH", juce::dontSendNotification);
    midiChannelLabel.attachToComponent(&midiChannelSlider, true);


    // === Row 2: Musical Context ===
    addAndMakeVisible(rootNoteCombo);
    rootNoteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "ROOT_NOTE", rootNoteCombo);

    addAndMakeVisible(rootNoteLabel);
    rootNoteLabel.setText("Root Note", juce::dontSendNotification);
    rootNoteLabel.attachToComponent(&rootNoteCombo, true);

    addAndMakeVisible(scaleCombo);
    scaleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "SCALE", scaleCombo);

    addAndMakeVisible(scaleLabel);
    scaleLabel.setText("Scale", juce::dontSendNotification);
    scaleLabel.attachToComponent(&scaleCombo, true);

    // Preset Controls
    presetDirectory = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                        .getChildFile(JucePlugin_Manufacturer)
                        .getChildFile(JucePlugin_Name)
                        .getChildFile("Presets");

    if (!presetDirectory.exists())
        presetDirectory.createDirectory();

    addAndMakeVisible(presetCombo);
    presetCombo.onChange = [this] { loadPreset(presetCombo.getSelectedId()); };
    scanForPresets(); // Initial scan

    addAndMakeVisible(savePresetButton);
    savePresetButton.setButtonText("Save Preset");
    savePresetButton.onClick = [this] { savePreset(); };
}

ToolbarComponent::~ToolbarComponent()
{
}

void ToolbarComponent::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::darkgrey);
    g.drawRect(getLocalBounds(), 1);
}

void ToolbarComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    auto row1 = bounds.removeFromTop(40);
    auto row2 = bounds.removeFromTop(40);

    // Layout Row 1
    startButton.setBounds(row1.removeFromLeft(80));
    row1.removeFromLeft(10);
    bpmSlider.setBounds(row1.removeFromLeft(150));
    row1.removeFromLeft(10);
    midiChannelSlider.setBounds(row1.removeFromLeft(120));

    // Layout Row 2
    rootNoteCombo.setBounds(row2.removeFromLeft(80));
    row2.removeFromLeft(10);
    scaleCombo.setBounds(row2.removeFromLeft(150));
    row2.removeFromLeft(10);
    presetCombo.setBounds(row2.removeFromLeft(150));
    row2.removeFromLeft(10);
    savePresetButton.setBounds(row2.removeFromLeft(100));
}

void ToolbarComponent::scanForPresets()
{
    presetFiles.clear();
    presetCombo.clear();

    auto presetFileArray = presetDirectory.findChildFiles(juce::File::findFiles, false, "*.xml");
    for (const auto& file : presetFileArray)
    {
        presetFiles.add(file.getFullPathName());
        presetCombo.addItem(file.getFileNameWithoutExtension(), presetFiles.size());
    }
}

void ToolbarComponent::savePreset()
{
    juce::FileChooser chooser("Save Preset", presetDirectory, "*.xml");

    if (chooser.browseForFileToSave(true))
    {
        juce::File file = chooser.getResult();
        auto state = audioProcessor.apvts.copyState();
        std::unique_ptr<juce::XmlElement> xml(state.createXml());

        if (xml != nullptr)
        {
            xml->writeTo(file);
            scanForPresets(); // Rescan to update the list
        }
    }
}

void ToolbarComponent::loadPreset(int presetId)
{
    if (presetId > 0 && presetId <= presetFiles.size())
    {
        juce::File presetFile(presetFiles[presetId - 1]);
        if (presetFile.existsAsFile())
        {
            std::unique_ptr<juce::XmlElement> xmlState(juce::XmlDocument::parse(presetFile));
            if (xmlState != nullptr)
            {
                if (xmlState->hasTagName(audioProcessor.apvts.state.getType()))
                {
                    audioProcessor.apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
                }
            }
        }
    }
}
