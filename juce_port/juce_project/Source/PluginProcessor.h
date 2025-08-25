#pragma once

#include <JuceHeader.h>
#include "Generators/RandomGenerator.h"
#include "Looper/Looper.h"
#include "Generators/GenerationParameters.h"

//==============================================================================
/**
*/
class CreativeMidiGeneratorAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    CreativeMidiGeneratorAudioProcessor();
    ~CreativeMidiGeneratorAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Параметры генерации
    void setNoteDensity(float density) { generationParams_.noteDensity = juce::jlimit(0.0f, 1.0f, density); }
    float getNoteDensity() const { return generationParams_.noteDensity; }

    void setVelocityRange(int min, int max) {
        generationParams_.velocityMin = juce::jlimit(0, 127, min);
        generationParams_.velocityMax = juce::jlimit(0, 127, max);
    }
    int getVelocityMin() const { return generationParams_.velocityMin; }
    int getVelocityMax() const { return generationParams_.velocityMax; }

    void setNoteRange(int min, int max) {
        generationParams_.minNote = juce::jlimit(0, 127, min);
        generationParams_.maxNote = juce::jlimit(0, 127, max);
    }
    int getNoteRangeMin() const { return generationParams_.minNote; }
    int getNoteRangeMax() const { return generationParams_.maxNote; }

    void setNoteLength(float length) { generationParams_.noteLength = juce::jlimit(0.1f, 4.0f, length); }
    float getNoteLength() const { return generationParams_.noteLength; }

    void setSyncToHost(bool sync) { generationParams_.syncToHost = sync; }
    bool getSyncToHost() const { return generationParams_.syncToHost; }

    const GenerationParameters& getGenerationParameters() const { return generationParams_; }

    // Новые методы для расширенных параметров генерации
    void setScaleRoot(int root) { generationParams_.rootNote = juce::jlimit(0, 127, root); }
    int getScaleRoot() const { return generationParams_.rootNote; }

    void setScaleType(int type) {
        // Предполагаем, что type соответствует enum ScaleType
        generationParams_.scaleType = static_cast<ScaleType>(juce::jlimit(0, 18, type));
    }
    int getScaleType() const { return static_cast<int>(generationParams_.scaleType); }

    void setRhythmComplexity(float complexity) { generationParams_.rhythmComplexity = juce::jlimit(0.0f, 1.0f, complexity); }
    float getRhythmComplexity() const { return generationParams_.rhythmComplexity; }

    void setTimeSignature(int numerator, int denominator) {
        generationParams_.timeSignatureNumerator = juce::jlimit(1, 16, numerator);
        generationParams_.timeSignatureDenominator = juce::jlimit(1, 16, denominator);
    }
    int getTimeSignatureNumerator() const { return generationParams_.timeSignatureNumerator; }
    int getTimeSignatureDenominator() const { return generationParams_.timeSignatureDenominator; }

    void setUseChordProgression(bool use) { generationParams_.useChordProgression = use; }
    bool getUseChordProgression() const { return generationParams_.useChordProgression; }

    void setChordProgression(const int chords[4]) {
        for (int i = 0; i < 4; ++i) {
            generationParams_.chordProgression[i] = juce::jlimit(0, 7, chords[i]);
        }
    }
    void getChordProgression(int chords[4]) const {
        for (int i = 0; i < 4; ++i) {
            chords[i] = generationParams_.chordProgression[i];
        }
    }

    void setHumanization(float amount) { generationParams_.humanization = juce::jlimit(0.0f, 1.0f, amount); }
    float getHumanization() const { return generationParams_.humanization; }

    void setOctaveRange(int range) { generationParams_.octaveRange = juce::jlimit(1, 4, range); }
    int getOctaveRange() const { return generationParams_.octaveRange; }

    //==============================================================================
    // Методы для управления лупером (для GUI)
    void startLooperRecording() { if (looper_) looper_->startRecording(); }
    void stopLooperRecording() { if (looper_) looper_->stopRecording(); }
    void startLooperPlayback() { if (looper_) looper_->startPlayback(); }
    void stopLooperPlayback() { if (looper_) looper_->stopPlayback(); }
    void clearLooper() { if (looper_) looper_->clear(); }

    void setLooperMode(LooperMode mode) { if (looper_) looper_->setMode(mode); }
    LooperMode getLooperMode() const { return looper_ ? looper_->getMode() : LooperMode::MidiLooper; }

    bool isLooperRecording() const { return looper_ ? looper_->isRecordingActive() : false; }
    bool isLooperPlaying() const { return looper_ ? looper_->isPlaybackActive() : false; }
    size_t getLooperNotesCount() const { return looper_ ? looper_->getRecordedNotesCount() : 0; }

    // Эффекты лупера
    void setLooperPitchShift(int semitones) { if (looper_) looper_->setPitchShift(semitones); }
    int getLooperPitchShift() const { return looper_ ? looper_->getPitchShift() : 0; }

    void setLooperPlaybackSpeed(float speed) { if (looper_) looper_->setPlaybackSpeed(speed); }
    float getLooperPlaybackSpeed() const { return looper_ ? looper_->getPlaybackSpeed() : 1.0f; }

    void setLooperReverse(bool reverse) { if (looper_) looper_->setReverse(reverse); }
    bool getLooperReverse() const { return looper_ ? looper_->getReverse() : false; }

private:
    //==============================================================================
    std::unique_ptr<RandomGenerator> randomGenerator_;
    std::unique_ptr<Looper> looper_;
    double currentBeat_ = 0.0;
    double samplesPerBeat_ = 0.0;
    double sampleRate_ = 44100.0;
    double currentBpm_ = 120.0;
    GenerationParameters generationParams_;
    
    // Получение BPM из DAW
    double getCurrentBpm() const;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CreativeMidiGeneratorAudioProcessor)
};