#include "RandomGenerator.h"

RandomGenerator::RandomGenerator()
{
    // Инициализация генератора случайных чисел
    random_.setSeed(juce::Time::currentTimeMillis());
}

std::pair<std::vector<std::pair<juce::MidiMessage, double>>, double>
RandomGenerator::generate(double currentBeat)
{
    std::vector<std::pair<juce::MidiMessage, double>> events;

    // Проверяем вероятность генерации ноты
    if (random_.nextFloat() > noteProbability_)
    {
        return {events, rate_}; // Нет ноты, просто ждем
    }

    auto availableNotes = getNotesInRange();
    if (availableNotes.empty())
    {
        return {events, rate_}; // Нет доступных нот
    }

    // Выбираем случайную ноту
    int randomIndex = random_.nextInt(availableNotes.size());
    int randomNote = availableNotes[randomIndex];

    // Вычисляем динамику
    int randomVelocity = calculateVelocity();

    // Создаем MIDI сообщение note_on
    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(channel_, randomNote, static_cast<uint8>(randomVelocity));
    float duration = getRandomDuration();
    events.push_back({noteOn, duration});

    // Опционально добавляем CC74 (brightness)
    if (addCC74_)
    {
        int ccValue = random_.nextInt(128);
        juce::MidiMessage cc74 = juce::MidiMessage::controllerEvent(channel_, 74, ccValue);
        events.push_back({cc74, 0.0}); // CC сообщения не имеют длительности
    }

    return {events, rate_};
}

void RandomGenerator::setParameter(const juce::String& paramId, float value)
{
    if (paramId == "minNote")
        minNote_ = static_cast<int>(value);
    else if (paramId == "maxNote")
        maxNote_ = static_cast<int>(value);
    else if (paramId == "maxVelocity")
        maxVelocity_ = static_cast<int>(value);
    else if (paramId == "velocityBias")
        velocityBias_ = value;
    else if (paramId == "noteProbability")
        noteProbability_ = value;
    else if (paramId == "rate")
        rate_ = value;
    else if (paramId == "channel")
        channel_ = static_cast<int>(value);
    else if (paramId == "addCC74")
        addCC74_ = (value > 0.5f);
}

float RandomGenerator::getParameter(const juce::String& paramId) const
{
    if (paramId == "minNote")
        return static_cast<float>(minNote_);
    else if (paramId == "maxNote")
        return static_cast<float>(maxNote_);
    else if (paramId == "maxVelocity")
        return static_cast<float>(maxVelocity_);
    else if (paramId == "velocityBias")
        return velocityBias_;
    else if (paramId == "noteProbability")
        return noteProbability_;
    else if (paramId == "rate")
        return rate_;
    else if (paramId == "channel")
        return static_cast<float>(channel_);
    else if (paramId == "addCC74")
        return addCC74_ ? 1.0f : 0.0f;

    return 0.0f;
}

void RandomGenerator::setScaleNotes(const std::vector<int>& notes)
{
    scaleNotes_ = notes;
}

std::vector<int> RandomGenerator::getNotesInRange() const
{
    if (scaleNotes_.empty())
    {
        // Если масштаб не установлен, возвращаем все ноты в диапазоне
        std::vector<int> allNotes;
        for (int note = minNote_; note <= maxNote_; ++note)
        {
            allNotes.push_back(note);
        }
        return allNotes;
    }
    else
    {
        // Фильтруем ноты масштаба по диапазону
        std::vector<int> filteredNotes;
        for (int note : scaleNotes_)
        {
            if (note >= minNote_ && note <= maxNote_)
            {
                filteredNotes.push_back(note);
            }
        }
        return filteredNotes;
    }
}

int RandomGenerator::calculateVelocity() const
{
    // Используем бета-распределение для смещения динамики
    // velocityBias_: 0.0 = тихие ноты, 1.0 = громкие ноты

    float bias = 1.0f - velocityBias_;
    float alpha, beta_param;

    if (bias < 0.5f)
    {
        alpha = 1.0f + (0.5f - bias) * 8.0f;
        beta_param = 1.0f;
    }
    else
    {
        alpha = 1.0f;
        beta_param = 1.0f + (bias - 0.5f) * 8.0f;
    }

    // Простая аппроксимация бета-распределения
    float u1 = random_.nextFloat();
    float u2 = random_.nextFloat();

    float x = 0.0f;
    if (alpha == 1.0f && beta_param == 1.0f)
    {
        x = u1; // Равномерное распределение
    }
    else
    {
        // Упрощенная аппроксимация
        x = (u1 * alpha + u2 * beta_param) / (alpha + beta_param);
    }

    int velocity = static_cast<int>(1 + x * (maxVelocity_ - 1));
    return juce::jlimit(1, maxVelocity_, velocity);
}

float RandomGenerator::getRandomDuration() const
{
    // Простая случайная длительность (можно улучшить)
    // В оригинале используется get_probabilistic_duration
    float durations[] = {0.25f, 0.5f, 1.0f, 1.5f, 2.0f};
    int randomIndex = random_.nextInt(5);
    return durations[randomIndex];
}