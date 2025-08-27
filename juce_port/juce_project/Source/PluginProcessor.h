#pragma once

#include <JuceHeader.h>
#include "Looper/Looper.h"
#include "Generators/BaseGenerator.h"
#include "Generators/RandomGenerator.h"
#include "Generators/EuclideanGenerator.h"
#include "Generators/DualEuclideanGenerator.h"
#include "Generators/RandomGeneratorV2.h"
#include "Theory/Scales.h"
#include <map>

//==============================================================================
/**
*/
class CreativeMidiGeneratorAudioProcessor  : public juce::AudioProcessor,
                                             public juce::AudioProcessorValueTreeState::Listener
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

    //==============================================================================
    // APVTS Listener Callback
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    void togglePlayback();
    bool isPlaying() const;

    //==============================================================================
    struct VariationData
    {
        float bassIntensity = 0.0f;
        float midIntensity = 0.0f;
        float highIntensity = 0.0f;
        int rootNote = 0;
        std::vector<int> scaleNotes;
    };

    // Looper Control Methods for UI
    void toggleLooperRecord();
    void toggleLooperPlay();
    void clearLooper();
    void captureFromGenerator();
    void quantizeLooper();
    void generateLooperVariation();
    void doubleLoop();
    void splitLoop();

    // These methods might interact directly with the looper or be handled via APVTS
    void setLooperMode(LooperMode mode); // Example, might be removed if fully APVTS driven
    LooperMode getLooperMode() const;

    bool isLooperRecording() const;
    bool isLooperPlaying() const;
    size_t getLooperNotesCount() const;

    // Looper Effects (might be refactored to use APVTS)
    void setLooperPitchShift(int semitones);
    int getLooperPitchShift() const;

    // Getters for Timeline UI
    const std::vector<Looper::RecordedNote>& getLooperNotes() const;
    double getLooperPlaybackProgress() const;

    struct LiveNote {
        int noteNumber;
        int velocity;
        double startTime; // in beats
        double duration;  // in beats
    };
    const std::vector<LiveNote>& getLiveNotes() const { return liveNotes; }
    double getCurrentBeat() const { return currentBeat_; }
    double getLooperDurationInBeats() const;

    void setLooperPlaybackSpeed(float speed) { if (looper_) looper_->setPlaybackSpeed(speed); }
    float getLooperPlaybackSpeed() const { return looper_ ? looper_->getPlaybackSpeed() : 1.0f; }

    void setLooperReverse(bool reverse) { if (looper_) looper_->setReverse(reverse); }
    bool getLooperReverse() const { return looper_ ? looper_->getReverse() : false; }

    // APVTS
    juce::AudioProcessorValueTreeState apvts;

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateActiveGenerator();
    void updateScale();

    std::map<int, std::unique_ptr<BaseGenerator>> availableGenerators;
    BaseGenerator* activeGenerator = nullptr;

    std::unique_ptr<Looper> looper_;
    std::vector<LiveNote> liveNotes;
    double currentBeat_ = 0.0;
    double samplesPerBeat_ = 0.0;
    double sampleRate_ = 44100.0;
    double currentBpm_ = 120.0;
    bool isPlaying_ = false;

    // Auto-Recapture State
    int autoRecapturePeriod_ = 0; // 0 for off
    int loopCounter_ = 0;
    double lastLoopPosition_ = 0.0;
    
    // Получение BPM из DAW
    double getCurrentBpm() const;

    bool sendAllNotesOff = false;

    //==============================================================================
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void playbackStateChanged(bool isPlaying) = 0;
        virtual void looperStateChanged(bool isPlaying) = 0;
    };

    void addListener(Listener* listener) { listeners_.add(listener); }
    void removeListener(Listener* listener) { listeners_.remove(listener); }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CreativeMidiGeneratorAudioProcessor)
private:
    double lastBeat_ = 0.0;
    enum class LooperAction { None, TogglePlay, ToggleRecord, Capture, Double, Split, Clear };
    void executePendingLooperAction();

    juce::ListenerList<Listener> listeners_;
    LooperAction pendingLooperAction = LooperAction::None;
};