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
#if JUCE_WINDOWS
    juce::Logger::writeToLog("Toolchain debug: Platform=" + juce::String(JUCE_WINDOWS ? "Windows" : "Other") +
                             ", 64-bit=" + juce::String(JUCE_64BIT ? "Yes" : "No") +
                             ", Pointer size=" + juce::String(sizeof(void*)) +
                             ", MSVC Version=" + juce::String(_MSC_VER));
#endif
    availableGenerators[0] = std::make_unique<RandomGenerator>();
    availableGenerators[1] = std::make_unique<EuclideanGenerator>();
    availableGenerators[2] = std::make_unique<DualEuclideanGenerator>();
    availableGenerators[3] = std::make_unique<RandomGeneratorV2>();

    apvts.addParameterListener("GENERATOR_TYPE", this);
    apvts.addParameterListener("ROOT_NOTE", this);
    apvts.addParameterListener("SCALE", this);
    apvts.addParameterListener("LOOPER_RECAPTURE_PERIOD", this);

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

void CreativeMidiGeneratorAudioProcessor::setCurrentProgram ([[maybe_unused]] int index)
{
}

const juce::String CreativeMidiGeneratorAudioProcessor::getProgramName ([[maybe_unused]] int index)
{
    return {};
}

void CreativeMidiGeneratorAudioProcessor::changeProgramName ([[maybe_unused]] int index, [[maybe_unused]] const juce::String& newName)
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
    juce::MidiBuffer thruMessages;
    if (apvts.getRawParameterValue("LOOPER_THROUGH")->load())
    {
        thruMessages.addEvents(midiMessages, 0, -1, 0);
    }

    buffer.clear();

    currentBpm_ = getCurrentBpm();
    samplesPerBeat_ = sampleRate_ * 60.0 / currentBpm_;
    double beatsPerSample = 1.0 / samplesPerBeat_;

    double lastBlockBeat = currentBeat_;
    currentBeat_ += buffer.getNumSamples() * beatsPerSample;

    if (isGeneratorSwitchPending_)
    {
        double nextQuarterNote = std::ceil(lastBlockBeat * 4.0) / 4.0;
        if (currentBeat_ >= nextQuarterNote && lastBlockBeat < nextQuarterNote)
        {
            juce::Logger::writeToLog("SWITCHING GENERATOR at beat: " + juce::String(currentBeat_));
            updateActiveGenerator();
            isGeneratorSwitchPending_ = false;
        }
    }

    if (pendingLooperAction != LooperAction::None)
    {
        if (currentBeat_ >= looperActionTriggerTime_ && lastBlockBeat < looperActionTriggerTime_)
        {
            executePendingLooperAction();
        }
    }

    // The MIDI processing logic is split into several stages:
    // 1. Generation: Create MIDI from the active source (looper or generator).
    // 2. Recording: Pass the correct sources to the looper if it's recording.
    // 3. Output: Combine the audible MIDI streams for the final output.
    // 4. MIDI Thru: Add any incoming MIDI messages if THRU is enabled.

    // --- 1. Generation Stage ---
    // We use separate buffers to hold MIDI from different sources.
    juce::MidiBuffer generatedMidi;
    juce::MidiBuffer looperPlaybackMidi;

    // Determine the primary audible source for this block.
    if (looper_ && looper_->isPlaybackActive())
    {
        // If the looper is playing, it's the primary source.
        bool isPadMode = apvts.getRawParameterValue("LOOPER_PAD_MODE")->load() > 0.5f;
        looperPlaybackMidi = looper_->getPlaybackBuffer(buffer.getNumSamples(), lastBlockBeat, currentBeat_, isPadMode);
    }
    else if (activeGenerator != nullptr && isPlaying_)
    {
        // Otherwise, if the main playback is active, the generator is the source.
        auto pendingNotes = activeGenerator->process(generatedMidi, apvts, sampleRate_, lastBlockBeat, currentBeat_, buffer.getNumSamples());
        juce::ignoreUnused(pendingNotes); // Note-off queue not yet implemented
    }

    // --- 2. Recording Stage ---
    if (looper_ && looper_->isRecordingActive())
    {
        // Per the new logic, the recording source depends on what is currently playing.

        // Always record direct MIDI input from the host/keyboard.
        for (const auto metadata : midiMessages)
        {
            looper_->recordNote(metadata.getMessage(), lastBlockBeat + (metadata.samplePosition * beatsPerSample));
        }

        // Now, add the secondary source (either the looper's own output or the generator's).
        if (looper_->isPlaybackActive())
        {
            // Case 1: Looper is playing. We record its output (overdubbing) and ignore the generator.
            for (const auto metadata : looperPlaybackMidi)
            {
                looper_->recordNote(metadata.getMessage(), lastBlockBeat + (metadata.samplePosition * beatsPerSample));
            }
        }
        else
        {
            // Case 2: Looper is not playing. We record the generator's output.
            for (const auto metadata : generatedMidi)
            {
                looper_->recordNote(metadata.getMessage(), lastBlockBeat + (metadata.samplePosition * beatsPerSample));
            }
        }
    }

    // --- 3. Output Stage ---
    // We start with a clean buffer, as the original `midiMessages` is for input.
    midiMessages.clear();

    // If an all-notes-off event is pending, send it and do nothing else.
    // This ensures a clean stop.
    if (sendAllNotesOff)
    {
        int channel = static_cast<int>(apvts.getRawParameterValue("MIDI_CHANNEL")->load());
        midiMessages.addEvent(juce::MidiMessage::allNotesOff(channel), 0);
        sendAllNotesOff = false;
        // Also send a sustain off message just in case PAD mode was active
        midiMessages.addEvent(juce::MidiMessage::controllerEvent(channel, 64, 0), 1);
        return; // Skip the rest of the processing for this block
    }

    if (sendSustainOff_.load())
    {
        int channel = static_cast<int>(apvts.getRawParameterValue("MIDI_CHANNEL")->load());
        midiMessages.addEvent(juce::MidiMessage::controllerEvent(channel, 64, 0), 0);
        sendSustainOff_ = false;
    }

    // Add the primary audible stream to the output.
    if (looper_ && looper_->isPlaybackActive())
    {
        midiMessages.addEvents(looperPlaybackMidi, 0, -1, 0);
    }
    else
    {
        midiMessages.addEvents(generatedMidi, 0, -1, 0);
    }

    // --- Handle Auto-Recapture ---
    if (looper_ && looper_->isPlaybackActive() && autoRecapturePeriod_ > 0)
    {
        double loopDuration = looper_->getDurationInBeats();
        if (loopDuration > 0)
        {
            double currentLoopPos = fmod(currentBeat_, loopDuration);
            if (currentLoopPos < lastLoopPosition_)
            {
                loopCounter_++;
                if (loopCounter_ >= autoRecapturePeriod_)
                {
                    loopCounter_ = 0;
                    scheduleLooperAction(LooperAction::Capture);
                }
            }
            lastLoopPosition_ = currentLoopPos;
        }
    }


    if (thruMessages.getNumEvents() > 0)
    {
        midiMessages.addEvents(thruMessages, 0, -1, 0);
    }
}

double CreativeMidiGeneratorAudioProcessor::getCurrentBpm() const
{
    if (auto* currentPlayHead = getPlayHead())
    {
        if (auto positionInfo = currentPlayHead->getPosition())
        {
            if (positionInfo->getBpm().hasValue())
            {
                return *positionInfo->getBpm();
            }
        }
    }
    return apvts.getRawParameterValue("BPM")->load();
}

double CreativeMidiGeneratorAudioProcessor::getLooperDurationInBeats() const
{
    if (looper_)
        return looper_->getDurationInBeats();
    return 0.0;
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

void CreativeMidiGeneratorAudioProcessor::scheduleLooperAction(LooperAction action)
{
    auto* quantizeParam = apvts.getRawParameterValue("LOOPER_ACTION_QUANTIZE");
    int quantizeChoice = quantizeParam ? static_cast<int>(quantizeParam->load()) : 3;

    if (quantizeChoice == 0)
    {
        pendingLooperAction = action;
        executePendingLooperAction();
        return;
    }

    pendingLooperAction = action;
    double nextTriggerTime = 0.0;

    double beatForCalc = currentBeat_;

    switch (quantizeChoice)
    {
        case 1: // Next 1/2
            nextTriggerTime = std::ceil(beatForCalc * 2.0) / 2.0;
            break;
        case 2: // Next Beat
            nextTriggerTime = std::ceil(beatForCalc);
            break;
        case 3: // Next Bar
            nextTriggerTime = std::ceil(beatForCalc / 4.0) * 4.0;
            break;
    }

    if (nextTriggerTime < currentBeat_ + 0.05)
    {
        switch (quantizeChoice)
        {
            case 1: nextTriggerTime += 0.5; break;
            case 2: nextTriggerTime += 1.0; break;
            case 3: nextTriggerTime += 4.0; break;
        }
    }

    looperActionTriggerTime_ = nextTriggerTime;
    juce::Logger::writeToLog("SCHEDULING action " + juce::String((int)action) + " for beat " + juce::String(looperActionTriggerTime_));
}

void CreativeMidiGeneratorAudioProcessor::toggleLooperRecord()
{
    scheduleLooperAction(LooperAction::ToggleRecord);
}

void CreativeMidiGeneratorAudioProcessor::toggleLooperPlay()
{
    scheduleLooperAction(LooperAction::TogglePlay);
}

void CreativeMidiGeneratorAudioProcessor::clearLooper()
{
    if (looper_) looper_->clear();
}

void CreativeMidiGeneratorAudioProcessor::captureFromGenerator()
{
    scheduleLooperAction(LooperAction::Capture);
}

void CreativeMidiGeneratorAudioProcessor::quantizeLooper()
{
    if (looper_)
    {
        auto* gridParam = apvts.getRawParameterValue("LOOPER_QUANTIZE_GRID");
        if (!gridParam) return;

        int choice = static_cast<int>(gridParam->load());
        double gridInBeats = 0.0;
        switch (choice) {
            case 1: gridInBeats = 1.0; break;
            case 2: gridInBeats = 0.5; break;
            case 3: gridInBeats = 0.25; break;
            case 4: gridInBeats = 0.125; break;
            case 5: gridInBeats = 0.0625; break;
            default: break;
        }

        if (gridInBeats > 0)
            looper_->quantize(gridInBeats);
        else
            looper_->unquantize();
    }
}

void CreativeMidiGeneratorAudioProcessor::generateLooperVariation()
{
    if (looper_)
    {
        float bassIntensity = apvts.getRawParameterValue("LOOPER_INTENSITY_BASS")->load();
        float midIntensity = apvts.getRawParameterValue("LOOPER_INTENSITY_MID")->load();
        float highIntensity = apvts.getRawParameterValue("LOOPER_INTENSITY_HIGH")->load();

        int rootNote = static_cast<int>(apvts.getRawParameterValue("ROOT_NOTE")->load());
        int scaleChoice = static_cast<int>(apvts.getRawParameterValue("SCALE")->load());
        ScaleType scaleType = static_cast<ScaleType>(scaleChoice);
        auto scaleNotes = Scales::getScaleNotes(rootNote, Scales::getScaleName(scaleType));

        looper_->generateVariations(bassIntensity, midIntensity, highIntensity, rootNote, scaleNotes);
    }
}

void CreativeMidiGeneratorAudioProcessor::doubleLoop()
{
    scheduleLooperAction(LooperAction::Double);
}

void CreativeMidiGeneratorAudioProcessor::splitLoop()
{
    scheduleLooperAction(LooperAction::Split);
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

    if (!isPlaying_)
    {
        // If we just stopped playback, send an all-notes-off message
        // to ensure no generator notes are left hanging.
        sendAllNotesOff = true;
    }

    listeners_.call([&](Listener& l) { l.playbackStateChanged(isPlaying_); });
}

bool CreativeMidiGeneratorAudioProcessor::isPlaying() const
{
    return isPlaying_;
}

void CreativeMidiGeneratorAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "GENERATOR_TYPE")
    {
        pendingGeneratorChoice_ = static_cast<int>(newValue);
        isGeneratorSwitchPending_ = true;
    }
    else if (parameterID == "ROOT_NOTE" || parameterID == "SCALE")
    {
        updateScale();
    }
    else if (parameterID == "LOOPER_RECAPTURE_PERIOD")
    {
        int choice = static_cast<int>(newValue);
        int periodMap[] = { 0, 2, 3, 4, 6, 8 };
        if (choice >= 0 && static_cast<size_t>(choice) < std::size(periodMap))
        {
            autoRecapturePeriod_ = periodMap[choice];
            loopCounter_ = 0;
        }
    }
    else if (parameterID == "LOOPER_PAD_MODE")
    {
        if (newValue < 0.5f) // If PAD mode was just turned OFF
        {
            sendSustainOff_ = true;
        }
    }
}

void CreativeMidiGeneratorAudioProcessor::updateActiveGenerator()
{
    sendAllNotesOff = true;
    if (activeGenerator)
    {
        activeGenerator->reset();
    }
    activeGenerator = availableGenerators[pendingGeneratorChoice_].get();
    updateScale();
}

void CreativeMidiGeneratorAudioProcessor::updateScale()
{
    if (activeGenerator)
    {
        auto rootNote = static_cast<int>(apvts.getRawParameterValue("ROOT_NOTE")->load());
        auto scaleChoice = static_cast<int>(apvts.getRawParameterValue("SCALE")->load());

        ScaleType scaleType = static_cast<ScaleType>(scaleChoice);

        activeGenerator->setScale(rootNote, Scales::getScaleNotes(rootNote, Scales::getScaleName(scaleType)));
    }
}

void CreativeMidiGeneratorAudioProcessor::executePendingLooperAction()
{
    if (!looper_) return;

    switch (pendingLooperAction)
    {
        case LooperAction::TogglePlay:
        {
            juce::Logger::writeToLog("ACTION: Executing TogglePlay.");
            sendAllNotesOff = true;
            if (looper_->isPlaybackActive())
            {
                juce::Logger::writeToLog("ACTION: Stopping looper playback.");
                looper_->stopPlayback();
            }
            else
            {
                juce::Logger::writeToLog("ACTION: Starting looper playback.");
                looper_->startPlayback();
            }
            listeners_.call([&](Listener& l) { l.looperStateChanged(looper_->isPlaybackActive()); });
            break;
        }
        case LooperAction::ToggleRecord:
        {
            juce::Logger::writeToLog("ACTION: Executing ToggleRecord.");
            if (looper_->isRecordingActive())
            {
                juce::Logger::writeToLog("ACTION: Stopping record, scheduling playback.");
                looper_->stopRecording();
                scheduleLooperAction(LooperAction::TogglePlay);
            }
            else
            {
                auto* lengthParam = apvts.getRawParameterValue("LOOPER_RECORD_LENGTH");
                double recordLengthInBeats = 16.0;
                if (lengthParam)
                {
                    int choice = static_cast<int>(lengthParam->load());
                    double lengthMap[] = { 4.0, 8.0, 16.0, 32.0, 64.0, 128.0, 256.0 };
                    if (choice >= 0 && static_cast<size_t>(choice) < std::size(lengthMap))
                        recordLengthInBeats = lengthMap[choice];
                }
                bool isOverdub = apvts.getRawParameterValue("LOOPER_RECORD_OVERDUB")->load() > 0.5f;
                juce::Logger::writeToLog("ACTION: Starting record for " + juce::String(recordLengthInBeats) + " beats. Overdub: " + (isOverdub ? "Yes" : "No"));
                looper_->startRecording(recordLengthInBeats, isOverdub, currentBeat_);
            }
            break;
        }
        case LooperAction::Capture:
        {
            sendAllNotesOff = true;
            juce::Logger::writeToLog("ACTION: Executing Capture.");
            if (activeGenerator && looper_)
            {
                auto* durationParam = apvts.getRawParameterValue("LOOPER_CAPTURE_DURATION");
                if (!durationParam) break;
                int choice = static_cast<int>(durationParam->load());
                double durationInBeats = 0.0;
                switch (choice) {
                    case 0: durationInBeats = 0.5; break;
                    case 1: durationInBeats = 1.0; break;
                    case 2: durationInBeats = 2.0; break;
                    case 3: durationInBeats = 4.0; break;
                    case 4: durationInBeats = 8.0; break;
                    case 5: durationInBeats = 16.0; break;
                }
                if (durationInBeats > 0)
                {
                    juce::Logger::writeToLog("ACTION: Generating pattern for " + juce::String(durationInBeats) + " beats.");
                    bool isOverdub = apvts.getRawParameterValue("LOOPER_CAPTURE_OVERDUB")->load() > 0.5f;
                    auto pattern = activeGenerator->getPattern(durationInBeats, apvts, sampleRate_);
                    juce::Logger::writeToLog("ACTION: Pattern generated with " + juce::String(pattern.getNumEvents()) + " events.");
                    looper_->loadFromMidiBuffer(pattern, sampleRate_, apvts.getRawParameterValue("BPM")->load(), isOverdub, durationInBeats);
                    looper_->startPlayback();
                    listeners_.call([&](Listener& l) { l.looperStateChanged(looper_->isPlaybackActive()); });
                }
            }
            break;
        }
        case LooperAction::Double:
            looper_->doubleLoop();
            break;
        case LooperAction::Split:
            looper_->splitLoop();
            break;
        case LooperAction::Clear:
            sendAllNotesOff = true;
            looper_->clear();
            break;
        case LooperAction::None:
            break;
    }
    pendingLooperAction = LooperAction::None;
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
    add(std::make_unique<juce::AudioParameterChoice>("SCALE", "Scale", Scales::getAvailableScaleNames(), 0));
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
    add(std::make_unique<juce::AudioParameterChoice>("RANDOM_V2_BASE_DURATION", "Random v2 Base Duration", juce::StringArray{"4 Bars", "2 Bars", "Whole Note (4/4)", "Dotted Half (3/4)", "Half Note (2/4)", "Quarter Note (1/4)"}, 2));
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
std::vector<LiveNote> CreativeMidiGeneratorAudioProcessor::getLiveNotes() const
{
    std::vector<LiveNote> notes;

    // It's crucial that this method is thread-safe, as it's called from the UI thread
    // while the audio thread might be modifying the data.
    // For now, we'll rely on the fact that `getNotes()` returns a copy or is otherwise safe,
    // and the generator's `recentNotes` is populated on the audio thread before this is called.
    // A proper implementation would use lock-free data structures.

    if (looper_ && looper_->isPlaybackActive())
    {
        // If looper is playing, its notes are the source of truth for the timeline.
        const auto& looperNotes = looper_->getNotes();
        notes.reserve(looperNotes.size());
        for (const auto& looperNote : looperNotes)
        {
            notes.push_back({
                looperNote.message.getNoteNumber(),
                looperNote.message.getVelocity(),
                looperNote.beatTime,
                looperNote.durationInBeats
            });
        }
    }
    else if (activeGenerator)
    {
        // If the generator is playing, get the notes it just generated.
        notes = activeGenerator->recentNotes;
    }

    return notes;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CreativeMidiGeneratorAudioProcessor();
}