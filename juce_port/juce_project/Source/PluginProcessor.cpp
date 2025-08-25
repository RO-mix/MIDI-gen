#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
CreativeMidiGeneratorAudioProcessor::CreativeMidiGeneratorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
    apvts(*this, nullptr, "Parameters", createParameterLayout())
#else
    : apvts(*this, nullptr, "Parameters", createParameterLayout())
#endif
{
    availableGenerators[0] = std::make_unique<RandomGenerator>();
    availableGenerators[1] = std::make_unique<EuclideanGenerator>();
    availableGenerators[2] = std::make_unique<DualEuclideanGenerator>();
    availableGenerators[3] = std::make_unique<RandomGeneratorV2>();

    apvts.addParameterListener("GENERATOR_TYPE", this);
    apvts.addParameterListener("ROOT_NOTE", this);
    apvts.addParameterListener("SCALE", this);

    updateActiveGenerator();

    looper_ = std::make_unique<Looper>();
}

CreativeMidiGeneratorAudioProcessor::~CreativeMidiGeneratorAudioProcessor()
{
    apvts.removeParameterListener("GENERATOR_TYPE", this);
    apvts.removeParameterListener("ROOT_NOTE", this);
    apvts.removeParameterListener("SCALE", this);
}

//==============================================================================
const juce::String CreativeMidiGeneratorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool CreativeMidiGeneratorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool CreativeMidiGeneratorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool CreativeMidiGeneratorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double CreativeMidiGeneratorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int CreativeMidiGeneratorAudioProcessor::getNumPrograms()
{
    return 1;
}

int CreativeMidiGeneratorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void CreativeMidiGeneratorAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String CreativeMidiGeneratorAudioProcessor::getProgramName (int index)
{
    return {};
}

void CreativeMidiGeneratorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void CreativeMidiGeneratorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sampleRate_ = sampleRate;
    looper_->prepareToPlay(sampleRate, samplesPerBlock);
}

void CreativeMidiGeneratorAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool CreativeMidiGeneratorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif
    return true;
  #endif
}
#endif

void CreativeMidiGeneratorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    currentBpm_ = getCurrentBpm();
    samplesPerBeat_ = sampleRate_ * 60.0 / currentBpm_;
    double beatsPerSample = 1.0 / samplesPerBeat_;

    juce::MidiBuffer generatedMidi;
    if (activeGenerator != nullptr && isPlaying_)
    {
        activeGenerator->process(generatedMidi, apvts, sampleRate_, currentBeat_);
    }

    currentBeat_ += buffer.getNumSamples() * beatsPerSample;

    midiMessages.addEvents(generatedMidi, 0, -1, 0);
}

double CreativeMidiGeneratorAudioProcessor::getCurrentBpm() const
{
    if (auto* playHead = getPlayHead())
    {
        if (auto positionInfo = playHead->getPosition())
        {
            if (positionInfo->getBpm().hasValue())
            {
                return *positionInfo->getBpm();
            }
        }
    }
    return apvts.getRawParameterValue("BPM")->load();
}

//==============================================================================
bool CreativeMidiGeneratorAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* CreativeMidiGeneratorAudioProcessor::createEditor()
{
    return new CreativeMidiGeneratorAudioProcessorEditor (*this);
}

//==============================================================================
void CreativeMidiGeneratorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void CreativeMidiGeneratorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// Looper Methods
//==============================================================================

void CreativeMidiGeneratorAudioProcessor::toggleLooperRecord()
{
    // TODO: Implement logic to toggle recording, possibly based on APVTS state
    if (looper_)
    {
        if (looper_->isRecordingActive())
            looper_->stopRecording();
        else
            looper_->startRecording();
    }
}

void CreativeMidiGeneratorAudioProcessor::toggleLooperPlay()
{
    // TODO: Implement logic to toggle playback
    if (looper_)
    {
        if (looper_->isPlaybackActive())
            looper_->stopPlayback();
        else
            looper_->startPlayback();
    }
}

void CreativeMidiGeneratorAudioProcessor::clearLooper()
{
    if (looper_) looper_->clear();
}

void CreativeMidiGeneratorAudioProcessor::captureFromGenerator()
{
    // TODO: Implement capture logic
}

void CreativeMidiGeneratorAudioProcessor::quantizeLooper()
{
    // TODO: Implement quantization logic
}

void CreativeMidiGeneratorAudioProcessor::generateLooperVariation()
{
    // TODO: Implement variation logic
}

void CreativeMidiGeneratorAudioProcessor::doubleLoop()
{
    // TODO: Implement loop doubling logic
}

void CreativeMidiGeneratorAudioProcessor::splitLoop()
{
    // TODO: Implement loop splitting logic
}

void CreativeMidiGeneratorAudioProcessor::setLooperMode(LooperMode mode)
{
    if (looper_) looper_->setMode(mode);
}

LooperMode CreativeMidiGeneratorAudioProcessor::getLooperMode() const
{
    return looper_ ? looper_->getMode() : LooperMode::MidiLooper;
}

bool CreativeMidiGeneratorAudioProcessor::isLooperRecording() const
{
    return looper_ ? looper_->isRecordingActive() : false;
}

bool CreativeMidiGeneratorAudioProcessor::isLooperPlaying() const
{
    return looper_ ? looper_->isPlaybackActive() : false;
}

size_t CreativeMidiGeneratorAudioProcessor::getLooperNotesCount() const
{
    return looper_ ? looper_->getRecordedNotesCount() : 0;
}

void CreativeMidiGeneratorAudioProcessor::setLooperPitchShift(int semitones)
{
    if (looper_) looper_->setPitchShift(semitones);
}

int CreativeMidiGeneratorAudioProcessor::getLooperPitchShift() const
{
    return looper_ ? looper_->getPitchShift() : 0;
}

const std::vector<Looper::RecordedNote>& CreativeMidiGeneratorAudioProcessor::getLooperNotes() const
{
    if (looper_)
        return looper_->getNotes();

    static const std::vector<Looper::RecordedNote> empty;
    return empty;
}

double CreativeMidiGeneratorAudioProcessor::getLooperPlaybackProgress() const
{
    return looper_ ? looper_->getPlaybackProgress() : 0.0;
}


void CreativeMidiGeneratorAudioProcessor::togglePlayback()
{
    isPlaying_ = !isPlaying_;
}

bool CreativeMidiGeneratorAudioProcessor::isPlaying() const
{
    return isPlaying_;
}

void CreativeMidiGeneratorAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "GENERATOR_TYPE")
    {
        updateActiveGenerator();
    }
    else if (parameterID == "ROOT_NOTE" || parameterID == "SCALE")
    {
        updateScale();
    }
}

void CreativeMidiGeneratorAudioProcessor::updateActiveGenerator()
{
    auto generatorChoice = static_cast<int>(apvts.getRawParameterValue("GENERATOR_TYPE")->load());
    activeGenerator = availableGenerators[generatorChoice].get();
    updateScale(); // Update scale for the newly selected generator
}

void CreativeMidiGeneratorAudioProcessor::updateScale()
{
    if (activeGenerator)
    {
        auto rootNote = static_cast<int>(apvts.getRawParameterValue("ROOT_NOTE")->load());
        auto scaleChoice = static_cast<int>(apvts.getRawParameterValue("SCALE")->load());

        // This is a simplified mapping. A more robust solution would be better.
        ScaleType scaleType = static_cast<ScaleType>(scaleChoice);

        activeGenerator->setScale(rootNote, Scales::getNotesInScale(rootNote, scaleType));
    }
}


juce::AudioProcessorValueTreeState::ParameterLayout CreativeMidiGeneratorAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto add = [&](auto&& param) {
        params.push_back(std::forward<decltype(param)>(param));
    };

    // === Global and Toolbar ===
    add(std::make_unique<juce::AudioParameterFloat>("BPM", "BPM", 20.0f, 300.0f, 120.0f));
    add(std::make_unique<juce::AudioParameterChoice>("ROOT_NOTE", "Root Note", juce::StringArray{"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"}, 0));
    add(std::make_unique<juce::AudioParameterChoice>("SCALE", "Scale", juce::StringArray{"Major", "Minor", "Dorian", "Mixolydian", "Lydian", "Phrygian", "Locrian", "Minor Pentatonic", "Major Pentatonic", "Chromatic"}, 0));
    add(std::make_unique<juce::AudioParameterInt>("MIDI_CHANNEL", "MIDI Channel", 1, 16, 1));
    add(std::make_unique<juce::AudioParameterChoice>("GENERATOR_TYPE", "Generator Type", juce::StringArray{"Random", "Euclidean", "Dual Euclidean", "Random v2.2"}, 0));

    // === Random Generator ===
    add(std::make_unique<juce::AudioParameterInt>("RANDOM_MIN_NOTE", "Random Min Note", 0, 127, 60));
    add(std::make_unique<juce::AudioParameterInt>("RANDOM_MAX_NOTE", "Random Max Note", 0, 127, 72));
    add(std::make_unique<juce::AudioParameterInt>("RANDOM_MAX_VELOCITY", "Random Max Velocity", 1, 127, 127));
    add(std::make_unique<juce::AudioParameterFloat>("RANDOM_VELOCITY_BIAS", "Random Velocity Bias", 0.0f, 1.0f, 0.5f));
    add(std::make_unique<juce::AudioParameterFloat>("RANDOM_NOTE_PROBABILITY", "Random Note Probability", 0.0f, 1.0f, 1.0f));
    add(std::make_unique<juce::AudioParameterFloat>("RANDOM_DURATION_BIAS", "Random Duration Bias", 0.0f, 1.0f, 0.5f));
    add(std::make_unique<juce::AudioParameterChoice>("RANDOM_RATE", "Random Rate", juce::StringArray{"4 bars", "2 bars", "1 bar", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64"}, 4));
    add(std::make_unique<juce::AudioParameterBool>("RANDOM_ADD_CC74", "Random Add CC74", false));

    // === Euclidean Generator ===
    add(std::make_unique<juce::AudioParameterInt>("EUCLIDEAN_STEPS", "Euclidean Steps", 1, 64, 16));
    add(std::make_unique<juce::AudioParameterInt>("EUCLIDEAN_PULSES", "Euclidean Pulses", 0, 64, 4));
    add(std::make_unique<juce::AudioParameterInt>("EUCLIDEAN_NOTE", "Euclidean Note", 0, 127, 60));
    add(std::make_unique<juce::AudioParameterInt>("EUCLIDEAN_VELOCITY", "Euclidean Velocity", 1, 127, 100));
    add(std::make_unique<juce::AudioParameterInt>("EUCLIDEAN_DEVIATION_RANGE", "Euclidean Deviation Range", 0, 36, 0));
    add(std::make_unique<juce::AudioParameterBool>("EUCLIDEAN_DEVIATION_BIPOLAR", "Euclidean Bipolar Deviation", false));
    add(std::make_unique<juce::AudioParameterChoice>("EUCLIDEAN_RATE", "Euclidean Rate", juce::StringArray{"4 bars", "2 bars", "1 bar", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64"}, 6));
    add(std::make_unique<juce::AudioParameterFloat>("EUCLIDEAN_DURATION_BIAS", "Euclidean Duration Bias", 0.0f, 1.0f, 0.5f));
    add(std::make_unique<juce::AudioParameterFloat>("EUCLIDEAN_NOTE_PROBABILITY", "Euclidean Note Probability", 0.0f, 1.0f, 1.0f));

    // === Dual Euclidean Generator ===
    add(std::make_unique<juce::AudioParameterInt>("DUAL_EUCLIDEAN_STEPS_A", "Dual Euclidean Steps A", 1, 64, 16));
    add(std::make_unique<juce::AudioParameterInt>("DUAL_EUCLIDEAN_PULSES_A", "Dual Euclidean Pulses A", 0, 64, 4));
    add(std::make_unique<juce::AudioParameterInt>("DUAL_EUCLIDEAN_NOTE_A", "Dual Euclidean Note A", 0, 127, 60));
    add(std::make_unique<juce::AudioParameterInt>("DUAL_EUCLIDEAN_VELOCITY_A", "Dual Euclidean Velocity A", 1, 127, 100));
    add(std::make_unique<juce::AudioParameterFloat>("DUAL_EUCLIDEAN_DURATION_BIAS_A", "Dual Euclidean Duration Bias A", 0.0f, 1.0f, 0.5f));
    add(std::make_unique<juce::AudioParameterInt>("DUAL_EUCLIDEAN_DEVIATION_A", "Dual Euclidean Deviation A", 0, 36, 0));
    add(std::make_unique<juce::AudioParameterBool>("DUAL_EUCLIDEAN_BIPOLAR_A", "Dual Euclidean Bipolar A", false));
    add(std::make_unique<juce::AudioParameterInt>("DUAL_EUCLIDEAN_STEPS_B", "Dual Euclidean Steps B", 1, 64, 15));
    add(std::make_unique<juce::AudioParameterInt>("DUAL_EUCLIDEAN_PULSES_B", "Dual Euclidean Pulses B", 0, 64, 4));
    add(std::make_unique<juce::AudioParameterInt>("DUAL_EUCLIDEAN_NOTE_B", "Dual Euclidean Note B", 0, 127, 67));
    add(std::make_unique<juce::AudioParameterInt>("DUAL_EUCLIDEAN_VELOCITY_B", "Dual Euclidean Velocity B", 1, 127, 100));
    add(std::make_unique<juce::AudioParameterFloat>("DUAL_EUCLIDEAN_DURATION_BIAS_B", "Dual Euclidean Duration Bias B", 0.0f, 1.0f, 0.5f));
    add(std::make_unique<juce::AudioParameterInt>("DUAL_EUCLIDEAN_DEVIATION_B", "Dual Euclidean Deviation B", 0, 36, 0));
    add(std::make_unique<juce::AudioParameterBool>("DUAL_EUCLIDEAN_BIPOLAR_B", "Dual Euclidean Bipolar B", false));
    add(std::make_unique<juce::AudioParameterFloat>("DUAL_EUCLIDEAN_NOTE_PROBABILITY", "Dual Euclidean Note Probability", 0.0f, 1.0f, 1.0f));
    add(std::make_unique<juce::AudioParameterChoice>("DUAL_EUCLIDEAN_RATE", "Dual Euclidean Rate", juce::StringArray{"4 bars", "2 bars", "1 bar", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64"}, 6));

    // === Random v2.2 Generator ===
    add(std::make_unique<juce::AudioParameterInt>("RANDOM_V2_MIN_NOTE", "Random v2 Min Note", 0, 127, 48));
    add(std::make_unique<juce::AudioParameterInt>("RANDOM_V2_MAX_NOTE", "Random v2 Max Note", 0, 127, 72));
    add(std::make_unique<juce::AudioParameterFloat>("RANDOM_V2_BURST_PROB", "Random v2 Burst Probability", 0.0f, 1.0f, 0.5f));
    add(std::make_unique<juce::AudioParameterFloat>("RANDOM_V2_NOTE_PROB", "Random v2 Note Probability", 0.0f, 1.0f, 1.0f));
    add(std::make_unique<juce::AudioParameterChoice>("RANDOM_V2_BASE_DURATION", "Random v2 Base Duration", juce::StringArray{"4 bars", "2 bars", "1 bar", "1/2 .", "1/2", "1/4"}, 2));
    add(std::make_unique<juce::AudioParameterChoice>("RANDOM_V2_ACCELERATION", "Random v2 Acceleration", juce::StringArray{"x1", "x2", "x4", "x8"}, 1));
    for (int i = 0; i < 8; ++i)
    {
        juce::String id = "RANDOM_V2_BURST_PATTERN_" + juce::String(i);
        juce::String name = "Random v2 Burst Pattern " + juce::String(i + 1);
        add(std::make_unique<juce::AudioParameterFloat>(id, name, 0.0f, 1.0f, 1.0f - (i * 0.1f)));
    }

    // === Looper ===
    add(std::make_unique<juce::AudioParameterBool>("LOOPER_THROUGH", "Looper Through", false));
    add(std::make_unique<juce::AudioParameterBool>("LOOPER_PAD_MODE", "Looper Pad Mode", false));
    add(std::make_unique<juce::AudioParameterChoice>("LOOPER_QUANTIZE_GRID", "Looper Quantize Grid", juce::StringArray{"Off", "1/4", "1/8", "1/16", "1/32", "1/64"}, 0));
    add(std::make_unique<juce::AudioParameterChoice>("LOOPER_CAPTURE_DURATION", "Looper Capture Duration", juce::StringArray{"1/8 bar", "1/4 bar", "1/2 bar", "1 bar", "2 bars", "4 bars"}, 3));
    add(std::make_unique<juce::AudioParameterBool>("LOOPER_CAPTURE_OVERDUB", "Looper Capture Overdub", false));
    add(std::make_unique<juce::AudioParameterChoice>("LOOPER_RECAPTURE_PERIOD", "Looper Recapture Period", juce::StringArray{"Off", "Every 2 loops", "Every 3 loops", "Every 4 loops", "Every 6 loops", "Every 8 loops"}, 0));
    add(std::make_unique<juce::AudioParameterChoice>("LOOPER_RECORD_LENGTH", "Looper Record Length", juce::StringArray{"1 bar", "2 bars", "4 bars", "8 bars", "16 bars", "32 bars", "64 bars"}, 2));
    add(std::make_unique<juce::AudioParameterBool>("LOOPER_RECORD_OVERDUB", "Looper Record Overdub", false));
    add(std::make_unique<juce::AudioParameterChoice>("LOOPER_ACTION_QUANTIZE", "Looper Action Quantize", juce::StringArray{"Instant", "Next 1/2", "Next Beat", "Next Bar"}, 3));
    add(std::make_unique<juce::AudioParameterFloat>("LOOPER_INTENSITY_BASS", "Looper Intensity Bass", 0.0f, 1.0f, 1.0f));
    add(std::make_unique<juce::AudioParameterFloat>("LOOPER_INTENSITY_MID", "Looper Intensity Mid", 0.0f, 1.0f, 1.0f));
    add(std::make_unique<juce::AudioParameterFloat>("LOOPER_INTENSITY_HIGH", "Looper Intensity High", 0.0f, 1.0f, 1.0f));

    return { params.begin(), params.end() };
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CreativeMidiGeneratorAudioProcessor();
}