#pragma once
#include <JuceHeader.h>

class BaseGenerator
{
public:
    virtual ~BaseGenerator() = default;

    // Основной метод генерации
    virtual std::pair<std::vector<std::pair<juce::MidiMessage, double>>, double>
    generate(double currentBeat) = 0;

    // Управление параметрами
    virtual void setParameter(const juce::String& paramId, float value) = 0;
    virtual float getParameter(const juce::String& paramId) const = 0;

    // Масштаб нот (эквивалент set_scale_notes в Python версии)
    virtual void setScaleNotes(const std::vector<int>& notes) { scaleNotes_ = notes; }
    virtual const std::vector<int>& getScaleNotes() const { return scaleNotes_; }

    // Обновление параметров (эквивалент update_params в Python версии)
    virtual void updateParams(const std::map<juce::String, float>& params)
    {
        for (const auto& [paramId, value] : params)
        {
            setParameter(paramId, value);
        }
    }

    // Смещение длительности (для совместимости с Python версией)
    virtual void setDurationBias(float bias) { durationBias_ = bias; }
    virtual float getDurationBias() const { return durationBias_; }

protected:
    // Общие члены для всех генераторов
    std::vector<int> scaleNotes_;     // Доступные ноты из текущего масштаба
    float durationBias_ = 0.5f;       // Смещение длительности (0.0 - длинные, 1.0 - короткие)
};