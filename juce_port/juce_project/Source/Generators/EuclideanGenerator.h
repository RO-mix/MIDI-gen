#pragma once
#include "BaseGenerator.h"

class EuclideanGenerator : public BaseGenerator
{
public:
    EuclideanGenerator();
    ~EuclideanGenerator() override = default;

    std::pair<std::vector<std::pair<juce::MidiMessage, double>>, double>
    generate(double currentBeat) override;

    void setParameter(const juce::String& paramId, float value) override;
    float getParameter(const juce::String& paramId) const override;

    // Специфические параметры EuclideanGenerator
    void setSteps(int steps) { steps_ = steps; updatePattern(); }
    void setPulses(int pulses) { pulses_ = pulses; updatePattern(); }
    void setNote(int note) { note_ = note; }
    void setVelocity(int velocity) { velocity_ = velocity; }
    void setChannel(int channel) { channel_ = channel; }
    void setRate(float rate) { rate_ = rate; }
    void setDeviationRange(int range) { deviationRange_ = range; }
    void setNoteProbability(float probability) { noteProbability_ = probability; }
    void setDeviationIsBipolar(bool bipolar) { deviationIsBipolar_ = bipolar; }

private:
    // Параметры генератора
    int steps_ = 16;                    // Количество шагов
    int pulses_ = 4;                    // Количество импульсов
    int note_ = 60;                     // Основная нота (C4)
    int velocity_ = 100;                // Динамика
    int channel_ = 0;                   // MIDI канал
    float rate_ = 0.25f;                // Темп (beats per step)
    int deviationRange_ = 0;            // Диапазон отклонения нот
    float noteProbability_ = 1.0f;      // Вероятность генерации ноты
    bool deviationIsBipolar_ = false;   // Биполярное отклонение

    // Внутреннее состояние
    int currentStep_ = -1;              // Текущий шаг
    std::vector<bool> pattern_;         // Евклидов паттерн

    // Вспомогательные методы
    void updatePattern();
    std::vector<int> getNotesInRange() const;

    // Random number generation
    mutable juce::Random random_;
};