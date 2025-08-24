#pragma once
#include "BaseGenerator.h"

class DualEuclideanGenerator : public BaseGenerator
{
public:
    DualEuclideanGenerator();
    ~DualEuclideanGenerator() override = default;

    std::pair<std::vector<std::pair<juce::MidiMessage, double>>, double>
    generate(double currentBeat) override;

    void setParameter(const juce::String& paramId, float value) override;
    float getParameter(const juce::String& paramId) const override;

    // Специфические параметры DualEuclideanGenerator
    // Machine A
    void setStepsA(int steps) { stepsA_ = steps; updatePatterns(); }
    void setPulsesA(int pulses) { pulsesA_ = pulses; updatePatterns(); }
    void setNoteA(int note) { noteA_ = note; }
    void setVelocityA(int velocity) { velocityA_ = velocity; }
    void setDeviationRangeA(int range) { deviationRangeA_ = range; }
    void setDeviationIsBipolarA(bool bipolar) { deviationIsBipolarA_ = bipolar; }

    // Machine B
    void setStepsB(int steps) { stepsB_ = steps; updatePatterns(); }
    void setPulsesB(int pulses) { pulsesB_ = pulses; updatePatterns(); }
    void setNoteB(int note) { noteB_ = note; }
    void setVelocityB(int velocity) { velocityB_ = velocity; }
    void setDeviationRangeB(int range) { deviationRangeB_ = range; }
    void setDeviationIsBipolarB(bool bipolar) { deviationIsBipolarB_ = bipolar; }

    // Global
    void setChannel(int channel) { channel_ = channel; }
    void setRate(float rate) { rate_ = rate; }
    void setNoteProbability(float probability) { noteProbability_ = probability; }

private:
    // Параметры Machine A
    int stepsA_ = 16;                   // Количество шагов для Machine A
    int pulsesA_ = 4;                   // Количество импульсов для Machine A
    int noteA_ = 60;                    // Основная нота для Machine A (C4)
    int velocityA_ = 100;               // Динамика для Machine A
    int deviationRangeA_ = 0;           // Диапазон отклонения для Machine A
    bool deviationIsBipolarA_ = false;  // Биполярное отклонение для Machine A

    // Параметры Machine B
    int stepsB_ = 15;                   // Количество шагов для Machine B
    int pulsesB_ = 4;                   // Количество импульсов для Machine B
    int noteB_ = 67;                    // Основная нота для Machine B (G4)
    int velocityB_ = 100;               // Динамика для Machine B
    int deviationRangeB_ = 0;           // Диапазон отклонения для Machine B
    bool deviationIsBipolarB_ = false;  // Биполярное отклонение для Machine B

    // Глобальные параметры
    int channel_ = 0;                   // MIDI канал
    float rate_ = 0.25f;                // Темп (beats per step)
    float noteProbability_ = 1.0f;      // Вероятность генерации нот

    // Внутреннее состояние
    int masterStep_ = -1;               // Мастер-счетчик шагов
    std::vector<bool> patternA_;        // Паттерн для Machine A
    std::vector<bool> patternB_;        // Паттерн для Machine B

    // Вспомогательные методы
    void updatePatterns();
    std::vector<int> getNotesInRange() const;
    int getNoteForMachine(int baseNote, int deviationRange, bool isBipolar, juce::Random& random) const;

    // Random number generation
    mutable juce::Random random_;
};