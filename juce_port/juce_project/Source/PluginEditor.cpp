#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
CreativeMidiGeneratorAudioProcessorEditor::CreativeMidiGeneratorAudioProcessorEditor (CreativeMidiGeneratorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (800, 600);

    // Setup GUI components
    setupSliders();
    setupLabels();
    setupScaleSelector();
    setupLooperControls();

    // Initial status update
    updateStatusLabel();
    updateButtonStates();
}

CreativeMidiGeneratorAudioProcessorEditor::~CreativeMidiGeneratorAudioProcessorEditor()
{
}

//==============================================================================
void CreativeMidiGeneratorAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // Draw a subtle border
    g.setColour (juce::Colours::grey.withAlpha(0.3f));
    g.drawRect (getLocalBounds(), 1);
}

void CreativeMidiGeneratorAudioProcessorEditor::resized()
{
    // Layout components
    auto area = getLocalBounds().reduced(10);
    auto titleHeight = 30;
    auto sliderHeight = 50;
    auto labelHeight = 20;
    auto buttonHeight = 40;
    auto sectionSpacing = 20;

    // Title
    titleLabel.setBounds(area.removeFromTop(titleHeight));

    area.removeFromTop(10); // spacing

    // Generator section title
    juce::Label generatorTitle("Generator", "Generator");
    generatorTitle.setBounds(area.removeFromTop(titleHeight));
    addAndMakeVisible(generatorTitle);

    area.removeFromTop(10);

    // Min note controls
    minNoteLabel.setBounds(area.removeFromTop(labelHeight));
    minNoteSlider.setBounds(area.removeFromTop(sliderHeight));

    area.removeFromTop(10);

    // Max note controls
    maxNoteLabel.setBounds(area.removeFromTop(labelHeight));
    maxNoteSlider.setBounds(area.removeFromTop(sliderHeight));

    area.removeFromTop(10);

    // Probability controls
    probabilityLabel.setBounds(area.removeFromTop(labelHeight));
    probabilitySlider.setBounds(area.removeFromTop(sliderHeight));

    area.removeFromTop(10);

    // Scale controls
    scaleLabel.setBounds(area.removeFromTop(labelHeight));
    scaleSelector.setBounds(area.removeFromTop(sliderHeight));

    area.removeFromTop(sectionSpacing);

    // Looper section
    looperTitleLabel.setBounds(area.removeFromTop(titleHeight));

    area.removeFromTop(10);

    // Looper control buttons (horizontal layout)
    auto buttonArea = area.removeFromTop(buttonHeight);
    auto buttonWidth = buttonArea.getWidth() / 4;

    recordButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
    playButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
    clearButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));

    area.removeFromTop(10);

    // Looper mode and status
    auto modeArea = area.removeFromTop(labelHeight);
    looperModeLabel.setBounds(modeArea.removeFromLeft(modeArea.getWidth() / 2));
    looperModeSelector.setBounds(modeArea);

    area.removeFromTop(10);

    // Effects controls
    pitchShiftLabel.setBounds(area.removeFromTop(labelHeight));
    pitchShiftSlider.setBounds(area.removeFromTop(sliderHeight));

    area.removeFromTop(10);

    playbackSpeedLabel.setBounds(area.removeFromTop(labelHeight));
    playbackSpeedSlider.setBounds(area.removeFromTop(sliderHeight));

    area.removeFromTop(10);

    // Reverse toggle and status
    auto reverseArea = area.removeFromTop(buttonHeight);
    reverseButton.setBounds(reverseArea.removeFromLeft(reverseArea.getWidth() / 2).reduced(5));
    statusLabel.setBounds(reverseArea.reduced(5));
}

void CreativeMidiGeneratorAudioProcessorEditor::setupSliders()
{
    // Min Note Slider
    addAndMakeVisible(minNoteSlider);
    minNoteSlider.setRange(0.0, 127.0, 1.0);
    minNoteSlider.setValue(60.0); // C4
    minNoteSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);

    // Max Note Slider
    addAndMakeVisible(maxNoteSlider);
    maxNoteSlider.setRange(0.0, 127.0, 1.0);
    maxNoteSlider.setValue(72.0); // C5
    maxNoteSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);

    // Probability Slider
    addAndMakeVisible(probabilitySlider);
    probabilitySlider.setRange(0.0, 1.0, 0.01);
    probabilitySlider.setValue(1.0);
    probabilitySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
}

void CreativeMidiGeneratorAudioProcessorEditor::setupLabels()
{
    // Title
    addAndMakeVisible(titleLabel);
    titleLabel.setText("Creative MIDI Generator", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);

    // Min Note Label
    addAndMakeVisible(minNoteLabel);
    minNoteLabel.setText("Min Note (MIDI):", juce::dontSendNotification);
    minNoteLabel.setJustificationType(juce::Justification::left);

    // Max Note Label
    addAndMakeVisible(maxNoteLabel);
    maxNoteLabel.setText("Max Note (MIDI):", juce::dontSendNotification);
    maxNoteLabel.setJustificationType(juce::Justification::left);

    // Probability Label
    addAndMakeVisible(probabilityLabel);
    probabilityLabel.setText("Note Probability:", juce::dontSendNotification);
    probabilityLabel.setJustificationType(juce::Justification::left);

    // Scale Label
    addAndMakeVisible(scaleLabel);
    scaleLabel.setText("Scale:", juce::dontSendNotification);
    scaleLabel.setJustificationType(juce::Justification::left);
}

void CreativeMidiGeneratorAudioProcessorEditor::setupScaleSelector()
{
    addAndMakeVisible(scaleSelector);
    scaleSelector.addItem("Chromatic", 1);
    scaleSelector.addItem("Major", 2);
    scaleSelector.addItem("Minor", 3);
    scaleSelector.addItem("Dorian", 4);
    scaleSelector.addItem("Mixolydian", 5);
    scaleSelector.setSelectedId(2); // Major by default
}

void CreativeMidiGeneratorAudioProcessorEditor::setupLooperControls()
{
    // Looper title
    addAndMakeVisible(looperTitleLabel);
    looperTitleLabel.setText("MIDI Looper", juce::dontSendNotification);
    looperTitleLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    looperTitleLabel.setJustificationType(juce::Justification::centred);

    // Record button
    addAndMakeVisible(recordButton);
    recordButton.setButtonText("Record");
    recordButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    recordButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    recordButton.onClick = [this] { recordButtonClicked(); };

    // Play button
    addAndMakeVisible(playButton);
    playButton.setButtonText("Play");
    playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
    playButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    playButton.onClick = [this] { playButtonClicked(); };

    // Clear button
    addAndMakeVisible(clearButton);
    clearButton.setButtonText("Clear");
    clearButton.setColour(juce::TextButton::buttonColourId, juce::Colours::orange);
    clearButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    clearButton.onClick = [this] { clearButtonClicked(); };

    // Mode selector
    addAndMakeVisible(looperModeSelector);
    looperModeSelector.addItem("MIDI Looper", 1);
    looperModeSelector.addItem("Generation Looper", 2);
    looperModeSelector.setSelectedId(audioProcessor.getLooperMode() == LooperMode::MidiLooper ? 1 : 2);
    looperModeSelector.onChange = [this] { looperModeChanged(); };

    addAndMakeVisible(looperModeLabel);
    looperModeLabel.setText("Mode:", juce::dontSendNotification);
    looperModeLabel.setJustificationType(juce::Justification::left);

    // Pitch shift
    addAndMakeVisible(pitchShiftSlider);
    pitchShiftSlider.setRange(-12.0, 12.0, 1.0);
    pitchShiftSlider.setValue(audioProcessor.getLooperPitchShift());
    pitchShiftSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    pitchShiftSlider.onValueChange = [this] {
        audioProcessor.setLooperPitchShift(static_cast<int>(pitchShiftSlider.getValue()));
    };

    addAndMakeVisible(pitchShiftLabel);
    pitchShiftLabel.setText("Pitch Shift (semitones):", juce::dontSendNotification);
    pitchShiftLabel.setJustificationType(juce::Justification::left);

    // Playback speed
    addAndMakeVisible(playbackSpeedSlider);
    playbackSpeedSlider.setRange(0.25, 4.0, 0.01);
    playbackSpeedSlider.setValue(audioProcessor.getLooperPlaybackSpeed());
    playbackSpeedSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    playbackSpeedSlider.onValueChange = [this] {
        audioProcessor.setLooperPlaybackSpeed(static_cast<float>(playbackSpeedSlider.getValue()));
    };

    addAndMakeVisible(playbackSpeedLabel);
    playbackSpeedLabel.setText("Playback Speed (x):", juce::dontSendNotification);
    playbackSpeedLabel.setJustificationType(juce::Justification::left);

    // Reverse toggle
    addAndMakeVisible(reverseButton);
    reverseButton.setButtonText("Reverse");
    reverseButton.setToggleState(audioProcessor.getLooperReverse(), juce::dontSendNotification);
    reverseButton.onStateChange = [this] {
        audioProcessor.setLooperReverse(reverseButton.getToggleState());
    };

    // Status label
    addAndMakeVisible(statusLabel);
    statusLabel.setText("Ready", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
}

void CreativeMidiGeneratorAudioProcessorEditor::recordButtonClicked()
{
    if (audioProcessor.isLooperRecording())
    {
        audioProcessor.stopLooperRecording();
    }
    else
    {
        audioProcessor.startLooperRecording();
    }
    updateButtonStates();
    updateStatusLabel();
}

void CreativeMidiGeneratorAudioProcessorEditor::playButtonClicked()
{
    if (audioProcessor.isLooperPlaying())
    {
        audioProcessor.stopLooperPlayback();
    }
    else
    {
        audioProcessor.startLooperPlayback();
    }
    updateButtonStates();
    updateStatusLabel();
}

void CreativeMidiGeneratorAudioProcessorEditor::clearButtonClicked()
{
    audioProcessor.clearLooper();
    updateButtonStates();
    updateStatusLabel();
}

void CreativeMidiGeneratorAudioProcessorEditor::looperModeChanged()
{
    auto selectedId = looperModeSelector.getSelectedId();
    LooperMode newMode = (selectedId == 1) ? LooperMode::MidiLooper : LooperMode::GenerationLooper;
    audioProcessor.setLooperMode(newMode);
    updateStatusLabel();
}

void CreativeMidiGeneratorAudioProcessorEditor::updateStatusLabel()
{
    juce::String statusText = "Ready";

    if (audioProcessor.isLooperRecording())
    {
        statusText = "RECORDING";
    }
    else if (audioProcessor.isLooperPlaying())
    {
        statusText = "PLAYING";
    }

    auto mode = audioProcessor.getLooperMode();
    juce::String modeText = (mode == LooperMode::MidiLooper) ? "MIDI Looper" : "Generation Looper";

    statusText += " | " + modeText;
    statusText += " | Notes: " + juce::String(audioProcessor.getLooperNotesCount());

    statusLabel.setText(statusText, juce::dontSendNotification);

    // Color coding for status
    if (audioProcessor.isLooperRecording())
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::red);
    else if (audioProcessor.isLooperPlaying())
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::green);
    else
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
}

void CreativeMidiGeneratorAudioProcessorEditor::updateButtonStates()
{
    bool isRecording = audioProcessor.isLooperRecording();
    bool isPlaying = audioProcessor.isLooperPlaying();

    recordButton.setButtonText(isRecording ? "Stop Rec" : "Record");
    playButton.setButtonText(isPlaying ? "Stop" : "Play");

    recordButton.setToggleState(isRecording, juce::dontSendNotification);
    playButton.setToggleState(isPlaying, juce::dontSendNotification);
}