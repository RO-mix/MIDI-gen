#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
CreativeMidiGeneratorAudioProcessor::CreativeMidiGeneratorAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
{
    // Инициализация генератора
    randomGenerator_ = std::make_unique<RandomGenerator>();
    
    // Передаем начальные параметры в генератор
    randomGenerator_->setGenerationParameters(generationParams_);

    // Инициализация лупера
    looper_ = std::make_unique<Looper>();
    looper_->setMode(LooperMode::GenerationLooper); // По умолчанию в режиме Generation Looper
}

CreativeMidiGeneratorAudioProcessor::~CreativeMidiGeneratorAudioProcessor()
{
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
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
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
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    sampleRate_ = sampleRate;
    currentBpm_ = getCurrentBpm();
    samplesPerBeat_ = sampleRate / (currentBpm_ / 60.0);
    currentBeat_ = 0.0;
}

void CreativeMidiGeneratorAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool CreativeMidiGeneratorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
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
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto numSamples = buffer.getNumSamples();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Генерация MIDI событий с интеграцией лупера
    if (randomGenerator_ != nullptr && looper_ != nullptr)
    {
        // Вычисляем количество бит в этом блоке
        double beatsInBlock = static_cast<double>(numSamples) / samplesPerBeat_;
        double blockStartTime = currentBeat_;
        double blockEndTime = currentBeat_ + beatsInBlock;

        // Генерируем события на основе текущего ритма
        auto [events, nextBeatOffset] = randomGenerator_->generate(currentBeat_);

        // Создаем буфер для сгенерированных событий
        juce::MidiBuffer generatedBuffer;

        // Добавляем MIDI события в буфер генерации
        for (const auto& [midiMessage, duration] : events)
        {
            // Вычисляем позицию в сэмплах для этого MIDI события
            int samplePosition = 0;

            // Добавляем MIDI сообщение в буфер генерации
            generatedBuffer.addEvent(midiMessage, samplePosition);

            // Если есть длительность, добавляем note_off
            if (duration > 0.0)
            {
                int noteOffSample = static_cast<int>(duration * samplesPerBeat_);
                if (noteOffSample < numSamples)
                {
                    auto noteOff = juce::MidiMessage::noteOff(midiMessage.getChannel(),
                                                             midiMessage.getNoteNumber());
                    generatedBuffer.addEvent(noteOff, noteOffSample);
                }
            }
        }

        // Интеграция с лупером
        juce::MidiBuffer finalOutput;

        if (looper_->getMode() == LooperMode::GenerationLooper)
        {
            // Режим Generation Looper: захватываем вывод генератора
            if (looper_->isRecordingActive())
            {
                looper_->recordMidiBuffer(generatedBuffer, blockStartTime);
            }

            // Получаем выход из лупера
            if (looper_->isPlaybackActive())
            {
                finalOutput = looper_->getPlaybackBuffer(numSamples, blockStartTime, blockEndTime);
            }
            else
            {
                // Если лупер не воспроизводит, передаем оригинальные события
                finalOutput = generatedBuffer;
            }
        }
        else if (looper_->getMode() == LooperMode::MidiLooper)
        {
            // Режим MIDI Looper: записываем оригинальные события
            if (looper_->isRecordingActive())
            {
                for (const auto& event : generatedBuffer)
                {
                    double eventTime = blockStartTime + (event.samplePosition / samplesPerBeat_);
                    looper_->recordNote(event.getMessage(), eventTime);
                }
            }

            // Получаем выход из лупера
            if (looper_->isPlaybackActive())
            {
                finalOutput = looper_->getPlaybackBuffer(numSamples, blockStartTime, blockEndTime);
            }
            else
            {
                finalOutput = generatedBuffer;
            }
        }

        // Добавляем финальный выход в MIDI буфер плагина
        for (const auto& event : finalOutput)
        {
            midiMessages.addEvent(event.getMessage(), event.samplePosition);
        }

        // Обновляем текущую позицию
        currentBeat_ += beatsInBlock;
    
        // Сбрасываем счетчик если он стал слишком большим
        if (currentBeat_ > 1000.0)
            currentBeat_ = 0.0;
    }
    
    // Обновляем BPM и samplesPerBeat_ на каждом блоке для синхронизации с хостом
    currentBpm_ = getCurrentBpm();
    samplesPerBeat_ = sampleRate_ / (currentBpm_ / 60.0);
    
    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        // ..do something to the data...
    }
}

//==============================================================================
bool CreativeMidiGeneratorAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* CreativeMidiGeneratorAudioProcessor::createEditor()
{
    return new CreativeMidiGeneratorAudioProcessorEditor (*this);
}

double CreativeMidiGeneratorAudioProcessor::getCurrentBpm() const
{
    if (auto* playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            if (auto bpm = position->getBpm())
                return *bpm;
        }
    }
    return 120.0; // Значение по умолчанию
}

//==============================================================================
void CreativeMidiGeneratorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void CreativeMidiGeneratorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CreativeMidiGeneratorAudioProcessor();
}