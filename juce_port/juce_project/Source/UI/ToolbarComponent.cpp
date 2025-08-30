#include "ToolbarComponent.h"

ToolbarComponent::ToolbarComponent(CreativeMidiGeneratorAudioProcessor& p)
    : audioProcessor(p)
{
    // === Row 1: Playback and MIDI ===
    addAndMakeVisible(bpmSlider);
    bpmSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    bpmSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    bpmSlider.setTooltip("Задает темп проекта в ударах в минуту (BPM).");
    bpmAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "BPM", bpmSlider);

    addAndMakeVisible(bpmLabel);
    bpmLabel.setText("BPM", juce::dontSendNotification);

    addAndMakeVisible(midiChannelSlider);
    midiChannelSlider.setSliderStyle(juce::Slider::IncDecButtons);
    midiChannelSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
    midiChannelSlider.setTooltip("Выбор MIDI-канала для вывода (1-16).");
    midiChannelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "MIDI_CHANNEL", midiChannelSlider);

    addAndMakeVisible(midiChannelLabel);
    midiChannelLabel.setText("MIDI CH", juce::dontSendNotification);


    // === Row 2: Musical Context ===
    addAndMakeVisible(rootNoteCombo);
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(audioProcessor.apvts.getParameter("ROOT_NOTE")))
    {
        rootNoteCombo.addItemList(choiceParam->choices, 1);
    }
    rootNoteCombo.setTooltip("Выбор основной ноты (тоники) для музыкального лада.");
    rootNoteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "ROOT_NOTE", rootNoteCombo);

    addAndMakeVisible(scaleCombo);
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(audioProcessor.apvts.getParameter("SCALE")))
    {
        scaleCombo.addItemList(choiceParam->choices, 1);
    }
    scaleCombo.setTooltip("Выбор музыкального лада, который будет использоваться генераторами.");
    scaleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "SCALE", scaleCombo);

    // Preset Controls
    presetDirectory = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                        .getChildFile(JucePlugin_Manufacturer)
                        .getChildFile(JucePlugin_Name)
                        .getChildFile("Presets");

    if (!presetDirectory.exists())
        presetDirectory.createDirectory();

    addAndMakeVisible(presetCombo);
    presetCombo.onChange = [this] { loadPreset(presetCombo.getSelectedId()); };
    presetCombo.setTooltip("Загрузить сохраненный пресет.");
    scanForPresets(); // Initial scan

    addAndMakeVisible(savePresetButton);
    savePresetButton.setButtonText("Save Preset");
    savePresetButton.setTooltip("Сохранить текущие настройки как новый пресет.");
    savePresetButton.onClick = [this] { savePreset(); };

    audioProcessor.addListener(this);
}

ToolbarComponent::~ToolbarComponent()
{
    audioProcessor.removeListener(this);
}

void ToolbarComponent::playbackStateChanged(bool isPlaying)
{
    // The main start/stop button is now in the Generator section.
    juce::ignoreUnused(isPlaying);
}

void ToolbarComponent::looperStateChanged(bool isPlaying)
{
    // The main start/stop button in the toolbar is not affected by the looper state.
    juce::ignoreUnused(isPlaying);
}

void ToolbarComponent::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::darkgrey);
    g.drawRect(getLocalBounds(), 1);
}

void ToolbarComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    // Use FlexBox for a more robust and cleaner layout
    juce::FlexBox fb;
    fb.flexWrap = juce::FlexBox::Wrap::wrap;
    fb.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    fb.alignContent = juce::FlexBox::AlignContent::flexStart;

    // Add all toolbar items to the FlexBox
    fb.items.add(juce::FlexItem(bpmLabel).withWidth(60));
    fb.items.add(juce::FlexItem(bpmSlider).withWidth(150).withMargin({0, 10, 0, 0}));
    fb.items.add(juce::FlexItem(midiChannelLabel).withWidth(60));
    fb.items.add(juce::FlexItem(midiChannelSlider).withWidth(100).withMargin({0, 20, 0, 0}));

    fb.items.add(juce::FlexItem(rootNoteCombo).withWidth(70));
    fb.items.add(juce::FlexItem(scaleCombo).withWidth(140).withMargin({0, 10, 0, 0}));
    fb.items.add(juce::FlexItem(presetCombo).withWidth(140).withMargin({0, 10, 0, 0}));
    fb.items.add(juce::FlexItem(savePresetButton).withWidth(100));

    fb.performLayout(bounds.toFloat());
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
