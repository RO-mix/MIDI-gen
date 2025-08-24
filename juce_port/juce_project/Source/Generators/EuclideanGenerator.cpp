#include "EuclideanGenerator.h"
#include "Duration.h"

EuclideanGenerator::EuclideanGenerator()
{
    // Инициализация генератора случайных чисел
    random_.setSeed(juce::Time::currentTimeMillis());
    updatePattern();
}

std::pair<std::vector<std::pair<juce::MidiMessage, double>>, double>
EuclideanGenerator::generate(double currentBeat)
{
    std::vector<std::pair<juce::MidiMessage, double>> events;

    // Проверяем глобальную вероятность генерации
    if (random_.nextFloat() > noteProbability_)
    {
        return {events, rate_};
    }

    if (pattern_.empty())
    {
        return {events, rate_};
    }

    // Переходим к следующему шагу
    currentStep_ = (currentStep_ + 1) % static_cast<int>(pattern_.size());

    // Проверяем, активен ли текущий шаг
    if (!pattern_[static_cast<size_t>(currentStep_)])
    {
        return {events, rate_}; // Этот шаг не активен
    }

    // Определяем ноту для генерации
    int noteToPlay = note_;
    if (deviationRange_ > 0)
    {
        auto availableNotes = getNotesInRange();
        if (!availableNotes.empty())
        {
            int baseNote = note_;
            int minNote = deviationIsBipolar_ ? (baseNote - deviationRange_) : baseNote;
            int maxNote = baseNote + deviationRange_;

            // Фильтруем доступные ноты по диапазону отклонения
            std::vector<int> possibleNotes;
            for (int note : availableNotes)
            {
                if (note >= minNote && note <= maxNote)
                {
                    possibleNotes.push_back(note);
                }
            }

            if (!possibleNotes.empty())
            {
                int randomIndex = random_.nextInt(static_cast<int>(possibleNotes.size()));
                noteToPlay = possibleNotes[static_cast<size_t>(randomIndex)];
            }
        }
    }

    // Создаем MIDI сообщение note_on
    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(channel_, noteToPlay, static_cast<uint8>(velocity_));
    float duration = Duration::getProbabilisticDuration(durationBias_, random_);
    events.push_back({noteOn, duration});

    return {events, rate_};
}

void EuclideanGenerator::setParameter(const juce::String& paramId, float value)
{
    if (paramId == "steps")
    {
        steps_ = static_cast<int>(value);
        updatePattern();
    }
    else if (paramId == "pulses")
    {
        pulses_ = static_cast<int>(value);
        updatePattern();
    }
    else if (paramId == "note")
        note_ = static_cast<int>(value);
    else if (paramId == "velocity")
        velocity_ = static_cast<int>(value);
    else if (paramId == "channel")
        channel_ = static_cast<int>(value);
    else if (paramId == "rate")
        rate_ = value;
    else if (paramId == "deviationRange")
        deviationRange_ = static_cast<int>(value);
    else if (paramId == "noteProbability")
        noteProbability_ = value;
    else if (paramId == "deviationIsBipolar")
        deviationIsBipolar_ = (value > 0.5f);
    else if (paramId == "durationBias")
        durationBias_ = value;
}

float EuclideanGenerator::getParameter(const juce::String& paramId) const
{
    if (paramId == "steps")
        return static_cast<float>(steps_);
    else if (paramId == "pulses")
        return static_cast<float>(pulses_);
    else if (paramId == "note")
        return static_cast<float>(note_);
    else if (paramId == "velocity")
        return static_cast<float>(velocity_);
    else if (paramId == "channel")
        return static_cast<float>(channel_);
    else if (paramId == "rate")
        return rate_;
    else if (paramId == "deviationRange")
        return static_cast<float>(deviationRange_);
    else if (paramId == "noteProbability")
        return noteProbability_;
    else if (paramId == "deviationIsBipolar")
        return deviationIsBipolar_ ? 1.0f : 0.0f;
    else if (paramId == "durationBias")
        return durationBias_;

    return 0.0f;
}

void EuclideanGenerator::updatePattern()
{
    // Ограничиваем параметры допустимыми значениями
    steps_ = juce::jmax(1, steps_);
    pulses_ = juce::jmax(0, juce::jmin(pulses_, steps_));

    if (pulses_ <= 0)
    {
        pattern_.clear();
        return;
    }

    // Алгоритм генерации евклидового ритма
    pattern_.resize(static_cast<size_t>(steps_), false);

    // Распределяем импульсы равномерно
    for (int i = 0; i < pulses_; ++i)
    {
        int position = (i * steps_) / pulses_;
        pattern_[static_cast<size_t>(position)] = true;
    }

    // Сбрасываем текущий шаг
    currentStep_ = -1;
}

std::vector<int> EuclideanGenerator::getNotesInRange() const
{
    // Возвращаем все ноты из масштаба или диапазон по умолчанию
    if (!scaleNotes_.empty())
    {
        return scaleNotes_;
    }

    // Если масштаб не установлен, возвращаем все MIDI ноты
    std::vector<int> allNotes;
    for (int note = 0; note < 128; ++note)
    {
        allNotes.push_back(note);
    }
    return allNotes;
}