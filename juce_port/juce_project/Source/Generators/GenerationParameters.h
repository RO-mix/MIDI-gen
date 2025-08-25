#pragma once
#include <JuceHeader.h>
#include "../Theory/Scales.h"

struct GenerationParameters
{
    // Основные параметры генерации
    int rootNote = 60;           // Корневая нота (C4 = 60)
    ScaleType scaleType = ScaleType::Major;  // Тип лада
    int minNote = 48;            // Минимальная нота
    int maxNote = 72;            // Максимальная нота
    
    // Параметры ритма
    float noteDensity = 0.5f;    // Плотность нот (0.0 - 1.0)
    float durationBias = 0.5f;   // Смещение длительности
    float velocityRange = 0.3f;  // Диапазон динамики
    
    // Параметры случайности
    int seed = 0;                // Сид для генератора
    float randomness = 0.7f;     // Уровень случайности
    
    // Дополнительные параметры
    bool usePolyphony = false;   // Использовать полифонию
    int maxPolyphony = 3;        // Максимальная полифония
    
    // Новые параметры для улучшенной генерации
    float rhythmComplexity = 0.5f;  // Сложность ритма (0.0 - 1.0)
    float humanization = 0.3f;      // Уровень человечности (0.0 - 1.0)
    int octaveRange = 2;            // Диапазон октав
    bool useChordProgression = false; // Использовать аккордовые прогрессии
    bool addCC74 = false;           // Добавлять CC74 (Brightness)
    float velocityMin = 0.3f;       // Минимальная динамика
    float velocityMax = 0.9f;       // Максимальная динамика
    float noteLength = 0.5f;        // Базовая длительность ноты
    bool syncToHost = true;         // Синхронизация с хостом
    int timeSignatureNumerator = 4;   // Числитель размера такта
    int timeSignatureDenominator = 4; // Знаменатель размера такта
    
    // Аккордовая прогрессия (4 аккорда)
    int chordProgression[4] = {0, 1, 2, 3}; // I - ii - iii - IV по умолчанию
    
    GenerationParameters() = default;
    
    // Методы для работы с параметрами
    void setRootNote(int note) { rootNote = juce::jlimit(0, 127, note); }
    void setScaleType(ScaleType type) { scaleType = type; }
    void setRange(int min, int max) 
    { 
        minNote = juce::jlimit(0, 127, min); 
        maxNote = juce::jlimit(0, 127, max); 
        if (minNote > maxNote) std::swap(minNote, maxNote);
    }
    
    // Получить доступные ноты из текущего масштава
    std::vector<int> getScaleNotes() const
    {
        juce::String scaleName = Scales::getScaleName(scaleType);
        return Scales::getScaleNotesInRange(rootNote, scaleName, minNote, maxNote);
    }
};