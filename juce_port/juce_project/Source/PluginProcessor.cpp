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
    samplesPerBeat_ = sampleRate / (120.0 / 60.0); // 120 BPM -> beats per second
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

    // Генерация MIDI событий
    if (randomGenerator_ != nullptr)
    {
        // Вычисляем количество бит в этом блоке
        double beatsInBlock = static_cast<double>(numSamples) / samplesPerBeat_;

        // Генерируем события на основе текущего ритма
        auto [events, nextBeatOffset] = randomGenerator_->generate(currentBeat_);

        // Добавляем MIDI события в буфер
        for (const auto& [midiMessage, duration] : events)
        {
            // Вычисляем позицию в сэмплах для этого MIDI события
            // Простая реализация - добавляем событие в начало блока
            int samplePosition = 0;

            // Добавляем MIDI сообщение в буфер
            midiMessages.addEvent(midiMessage, samplePosition);

            // Если есть длительность, добавляем note_off
            if (duration > 0.0)
            {
                // Вычисляем позицию для note_off
                int noteOffSample = static_cast<int>(duration * samplesPerBeat_);
                if (noteOffSample < numSamples)
                {
                    auto noteOff = juce::MidiMessage::noteOff(midiMessage.getChannel(),
                                                             midiMessage.getNoteNumber());
                    midiMessages.addEvent(noteOff, noteOffSample);
                }
            }
        }

        // Обновляем текущую позицию
        currentBeat_ += beatsInBlock;

        // Сбрасываем счетчик если он стал слишком большим
        if (currentBeat_ > 1000.0)
            currentBeat_ = 0.0;
    }

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