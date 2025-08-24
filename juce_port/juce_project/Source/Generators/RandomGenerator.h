#pragma once
#include "BaseGenerator.h"
#include "../Theory/Scales.h"

class RandomGenerator : public BaseGenerator
{
public:
    RandomGenerator();
    ~RandomGenerator() override = default;

    std::pair<std::vector<std::pair<juce::MidiMessage, double>>, double>
    generate(double currentBeat) override;

    void setParameter(const juce::String& paramId, float value) override;
    float getParameter(const juce::String& paramId) const override;

    // Специфические параметры RandomGenerator
    void setScaleNotes(const std::vector<int>& notes);
    void setMinNote(int minNote) { minNote_ = minNote; }
    void setMaxNote(int maxNote) { maxNote_ = maxNote; }
    void setMaxVelocity(int maxVelocity) { maxVelocity_ = maxVelocity; }
    void setVelocityBias(float bias) { velocityBias_ = bias; }
    void setNoteProbability(float probability) { noteProbability_ = probability; }
    void setRate(float rate) { rate_ = rate; }
    void setChannel(int channel) { channel_ = channel; }
    void setAddCC74(bool add) { addCC74_ = add; }

private:
    // Параметры генератора
    int minNote_ = 60;           // Минимальная нота (C4)
    int maxNote_ = 72;           // Максимальная нота (C5)
    int maxVelocity_ = 127;      // Максимальная динамика
    float velocityBias_ = 0.5f;  // Смещение динамики (0.0 - тихие, 1.0 - громкие)
    float noteProbability_ = 1.0f; // Вероятность генерации ноты
    float rate_ = 1.0f;          // Темп генерации (beats)
    int channel_ = 0;            // MIDI канал
    bool addCC74_ = false;       // Добавлять CC74 (brightness)

    // Масштаб и доступные ноты
    std::vector<int> scaleNotes_;
    Scales scales_;

    // Вспомогательные методы
    std::vector<int> getNotesInRange() const;
    int calculateVelocity() const;
    float getRandomDuration() const;

    // Random number generation
    mutable juce::Random random_;
};