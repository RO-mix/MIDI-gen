#pragma once
#include "BaseGenerator.h"
#include "../Theory/Scales.h"
#include "GenerationParameters.h"

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
    void setMinVelocity(int minVelocity) { velocityMin_ = minVelocity; }
    void setMaxVelocity(int maxVelocity) { velocityMax_ = maxVelocity; }
    void setVelocityBias(float bias) { velocityBias_ = bias; }
    void setNoteProbability(float probability) { noteProbability_ = probability; }
    void setRate(float rate) { rate_ = rate; }
    void setChannel(int channel) { channel_ = channel; }
    void setAddCC74(bool add) { addCC74_ = add; }
    void setDurationBias(float bias) { durationBias_ = bias; }
    
    // Новый метод для установки параметров из структуры
    void setGenerationParameters(const GenerationParameters& params);

private:
    // Параметры генератора
    int minNote_ = 60;           // Минимальная нота (C4)
    int maxNote_ = 72;           // Максимальная нота (C5)
    int velocityMin_ = 60;       // Минимальная динамика
    int velocityMax_ = 100;      // Максимальная динамика
    float velocityBias_ = 0.5f;  // Смещение динамики (0.0 - тихие, 1.0 - громкие)
    float noteProbability_ = 1.0f; // Вероятность генерации ноты
    float rate_ = 1.0f;          // Темп генерации (beats)
    int channel_ = 0;            // MIDI канал
    float durationBias_ = 0.5f;  // Смещение длительности (0.0 - длинные, 1.0 - короткие)
    bool addCC74_ = false;       // Добавлять CC74 (brightness)

    // Новые параметры для улучшенной генерации
    float rhythmComplexity_ = 0.5f;  // Сложность ритма
    float humanization_ = 0.3f;      // Уровень человечности
    int octaveRange_ = 2;            // Диапазон октав
    bool useChordProgression_ = false; // Использовать аккордовые прогрессии
    int chordProgression_[4] = {0, 1, 2, 3}; // Аккордовая прогрессия (4 аккорда)
    int currentChordIndex_ = 0;      // Текущий аккорд в прогрессии
    std::vector<int> scales_;          // Доступные ноты в масштабе

    // Дополнительные параметры для расширенной функциональности
    float swingAmount_ = 0.0f;           // Количество свинга (0.0 - 1.0)
    float accentIntensity_ = 0.5f;       // Интенсивность акцентов
    int grooveTemplate_ = 0;             // Шаблон грува (0 = off, 1-8 = presets)
    float microTiming_ = 0.0f;           // Микротайминг (-1.0 - 1.0)
    float probabilityCurve_ = 0.5f;      // Кривая вероятности
    float velocityResponse_ = 0.5f;      // Отклик на динамику
    float noteLengthVariation_ = 0.0f;   // Вариация длительности нот
    
    // Параметры для арпеджио
    int arpMode_ = 0;                    // Режим арпеджио (0 = off, 1-4 = modes)
    int arpDirection_ = 0;               // Направление арпеджио (0 = up, 1 = down, 2 = up/down)
    int arpOctaves_ = 1;                 // Количество октав для арпеджио
    float arpRate_ = 0.5f;               // Скорость арпеджио
    
    // Параметры для аккордов
    int chordVoicing_ = 0;               // Тип озвучки аккордов
    int chordInversions_ = 0;            // Количество инверсий
    float strumSpeed_ = 0.5f;            // Скорость перебора
    int strumDirection_ = 0;             // Направление перебора (0 = up, 1 = down)

    GenerationParameters params_;
    
    // Устаревшие параметры (оставлены для обратной совместимости)
    int scaleRoot_ = 60;         // Корень тональности
    int scaleType_ = 0;          // Тип тональности
    
    // Вспомогательные методы
    std::vector<int> getNotesInRange() const;
    std::vector<int> getNotesInChord() const;
    int calculateVelocity() const;
    float getRandomDuration() const;
    float getRhythmVariation() const;
    int applyHumanization(int note) const;
    void updateScaleNotes();
    
    // Методы для работы с аккордовыми прогрессиями
    void setChordProgression(const std::vector<std::vector<int>>& progression);
    void setCurrentChordIndex(int index) { currentChordIndex_ = index; }
    int getCurrentChordIndex() const { return currentChordIndex_; }

    // Random number generation
    mutable juce::Random random_;
};