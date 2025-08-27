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

    addAndMakeVisible(midiChannelSlider);
    midiChannelSlider.setSliderStyle(juce::Slider::IncDecButtons);
    midiChannelSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
    midiChannelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "MIDI_CHANNEL", midiChannelSlider);

    addAndMakeVisible(midiChannelLabel);
    midiChannelLabel.setText("MIDI CH", juce::dontSendNotification);


    // === Row 2: Musical Context ===
    addAndMakeVisible(rootNoteCombo);
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(audioProcessor.apvts.getParameter("ROOT_NOTE")))
    {
        rootNoteCombo.addItemList(choiceParam->choices, 1);
    }
    rootNoteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "ROOT_NOTE", rootNoteCombo);

    addAndMakeVisible(rootNoteLabel);
    rootNoteLabel.setText("Root Note", juce::dontSendNotification);

    addAndMakeVisible(scaleCombo);
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(audioProcessor.apvts.getParameter("SCALE")))
    {
        scaleCombo.addItemList(choiceParam->choices, 1);
    }
    scaleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "SCALE", scaleCombo);

    addAndMakeVisible(scaleLabel);
    scaleLabel.setText("Scale", juce::dontSendNotification);

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
    auto row = bounds.removeFromTop(40);
    int x = 0;
    int spacing = 10;
    int labelWidth = 60;
    int componentWidth = 100;

    // --- Row 1: Playback and MIDI ---
    startButton.setBounds(row.removeFromLeft(80));
    row.removeFromLeft(spacing);

    bpmLabel.setBounds(row.removeFromLeft(labelWidth));
    bpmSlider.setBounds(row.removeFromLeft(componentWidth + 50)); // Wider for text box
    row.removeFromLeft(spacing);

    midiChannelLabel.setBounds(row.removeFromLeft(labelWidth));
    midiChannelSlider.setBounds(row.removeFromLeft(componentWidth));

    // --- Row 2: Musical Context & Presets ---
    row = bounds.removeFromTop(40);

    rootNoteLabel.setBounds(row.removeFromLeft(labelWidth));
    rootNoteCombo.setBounds(row.removeFromLeft(componentWidth - 20));
    row.removeFromLeft(spacing);

    scaleLabel.setBounds(row.removeFromLeft(labelWidth));
    scaleCombo.setBounds(row.removeFromLeft(componentWidth + 20));
    row.removeFromLeft(spacing);

    presetCombo.setBounds(row.removeFromLeft(componentWidth + 20));
    row.removeFromLeft(spacing);
    savePresetButton.setBounds(row.removeFromLeft(componentWidth));
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
    fileChooser = std::make_unique<juce::FileChooser>("Save Preset",
                                                      presetDirectory,
                                                      "*.xml");

    auto chooserFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting;

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser)
    {
        juce::File file = chooser.getResult();
        if (file != juce::File{})
        {
            auto state = audioProcessor.apvts.copyState();
            std::unique_ptr<juce::XmlElement> xml(state.createXml());

            if (xml != nullptr)
            {
                xml->writeTo(file);
                scanForPresets(); // Rescan to update the list
            }
        }
    });
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
