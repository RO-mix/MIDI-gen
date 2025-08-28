#pragma once
#include <JuceHeader.h>
#include <set>

enum class LooperMode
{
    MidiLooper,      // Классический MIDI лупер
    GenerationLooper // Лупер для генераторов с эффектами
};

class Looper
{
public:
    struct RecordedNote {
        juce::MidiMessage message;
        double beatTime;
        double durationInBeats = 0.25; // Default duration
    };

    Looper();

    void prepareToPlay(double sampleRate, int samplesPerBlock);

    // Основные методы управления
    void recordNote(const juce::MidiMessage& message, double beatTime);
    void recordMidiBuffer(const juce::MidiBuffer& buffer, double startTime);
    void loadFromMidiBuffer(const juce::MidiBuffer& buffer, double sampleRate, double bpm, bool isOverdub, double requestedDuration);
    void startPlayback();
    void stopPlayback();
    void clear();

    // Буфер воспроизведения
    juce::MidiBuffer getPlaybackBuffer(int numSamples, double startTime, double endTime, bool isPadMode);

    // Управление режимом
    void setMode(LooperMode mode);
    LooperMode getMode() const { return currentMode; }

    // Управление лупом
    void quantize(double gridInBeats);
    void unquantize();
    void generateVariations(float bassIntensity, float midIntensity, float highIntensity, int rootNote, const std::vector<int>& scaleNotes);
    juce::MidiBuffer doubleLoop();
    juce::MidiBuffer splitLoop(bool keepFirstHalf = true);
    void setLoopPoints(double start, double end);
    double getLoopStart() const { return loopStart; }
    double getLoopEnd() const { return loopEnd; }

    // Параметры генерации лупера
    void setBufferSize(double sizeInBeats) { bufferSize = sizeInBeats; }
    double getBufferSize() const { return bufferSize; }

    // Эффекты для Generation Looper
    void setPlaybackSpeed(float speed) { playbackSpeed = speed; }
    float getPlaybackSpeed() const { return playbackSpeed; }

    void setPitchShift(int semitones) { pitchShift = semitones; }
    int getPitchShift() const { return pitchShift; }

    void setReverse(bool shouldReverse) { reverse = shouldReverse; }
    bool getReverse() const { return reverse; }

    // Геттеры состояния
    bool isRecordingActive() const { return isRecording; }
    bool isPlaybackActive() const { return isPlaying; }
    size_t getRecordedNotesCount() const { return recordedNotes.size(); }

    // Getters for UI
    const std::vector<RecordedNote>& getNotes() const { return recordedNotes; }
    double getPlaybackProgress() const;
    double getDurationInBeats() const { return loopEnd - loopStart; }


    // Управление записью/воспроизведением
    void startRecording(double maxDuration, bool isOverdub, double currentBeat);
    void stopRecording();
    void setRecording(bool recording);

private:

    // Режим работы
    LooperMode currentMode = LooperMode::MidiLooper;

    // Данные записи
    std::vector<RecordedNote> recordedNotes;
    std::vector<RecordedNote> pristine_loop_;
    std::vector<RecordedNote> pendingNotes; // For calculating duration
    std::set<int> currentlyPlayingNotes;
    juce::MidiBuffer generationBuffer; // Для Generation Looper

    // Состояние
    bool isRecording = false;
    bool isPlaying = false;
    double playbackHead_ = 0.0;
    double recordingStartTime_ = 0.0;
    double maxRecordLengthBeats_ = 16.0; // Default to 4 bars

    // Параметры лупа
    double loopStart = 0.0;
    double loopEnd = 4.0; // 4 beats
    double bufferSize = 4.0; // Размер буфера в битах

    // Эффекты для Generation Looper
    float playbackSpeed = 1.0f;
    int pitchShift = 0;
    bool reverse = false;

    // Внутренние методы
    juce::MidiBuffer processMidiLooperBuffer(int numSamples, double startTime, double endTime, bool isPadMode);
    juce::MidiBuffer processGenerationLooperBuffer(int numSamples, double startTime, double endTime);
    juce::MidiMessage applyEffects(const juce::MidiMessage& message, double timeOffset);
};