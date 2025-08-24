#include "DualEuclideanGenerator.h"
#include "Duration.h"

DualEuclideanGenerator::DualEuclideanGenerator()
{
    // Инициализация генератора случайных чисел
    random_.setSeed(juce::Time::currentTimeMillis());
    updatePatterns();
}

std::pair<std::vector<std::pair<juce::MidiMessage, double>>, double>
DualEuclideanGenerator::generate(double currentBeat)
{
    std::vector<std::pair<juce::MidiMessage, double>> events;

    // Проверяем глобальную вероятность генерации
    if (random_.nextFloat() > noteProbability_)
    {
        return {events, rate_};
    }

    // Переходим к следующему мастер-шагу
    masterStep_++;

    // --- Machine A ---
    if (!patternA_.empty())
    {
        size_t stepA = static_cast<size_t>(masterStep_) % patternA_.size();
        if (patternA_[stepA])
        {
            // Определяем ноту для Machine A
            int noteA = getNoteForMachine(noteA_, deviationRangeA_, deviationIsBipolarA_, random_);

            // Создаем MIDI сообщение для Machine A
            juce::MidiMessage noteOnA = juce::MidiMessage::noteOn(channel_, noteA, static_cast<uint8>(velocityA_));
            float durationA = Duration::getProbabilisticDuration(durationBias_, random_);
            events.push_back({noteOnA, durationA});
        }
    }

    // --- Machine B ---
    if (!patternB_.empty())
    {
        size_t stepB = static_cast<size_t>(masterStep_) % patternB_.size();
        if (patternB_[stepB])
        {
            // Определяем ноту для Machine B
            int noteB = getNoteForMachine(noteB_, deviationRangeB_, deviationIsBipolarB_, random_);

            // Создаем MIDI сообщение для Machine B
            juce::MidiMessage noteOnB = juce::MidiMessage::noteOn(channel_, noteB, static_cast<uint8>(velocityB_));
            float durationB = Duration::getProbabilisticDuration(durationBias_, random_);
            events.push_back({noteOnB, durationB});
        }
    }

    return {events, rate_};
}

void DualEuclideanGenerator::setParameter(const juce::String& paramId, float value)
{
    bool patternChanged = false;

    // Machine A parameters
    if (paramId == "stepsA")
    {
        stepsA_ = static_cast<int>(value);
        patternChanged = true;
    }
    else if (paramId == "pulsesA")
    {
        pulsesA_ = static_cast<int>(value);
        patternChanged = true;
    }
    else if (paramId == "noteA")
        noteA_ = static_cast<int>(value);
    else if (paramId == "velocityA")
        velocityA_ = static_cast<int>(value);
    else if (paramId == "deviationRangeA")
        deviationRangeA_ = static_cast<int>(value);
    else if (paramId == "deviationIsBipolarA")
        deviationIsBipolarA_ = (value > 0.5f);

    // Machine B parameters
    else if (paramId == "stepsB")
    {
        stepsB_ = static_cast<int>(value);
        patternChanged = true;
    }
    else if (paramId == "pulsesB")
    {
        pulsesB_ = static_cast<int>(value);
        patternChanged = true;
    }
    else if (paramId == "noteB")
        noteB_ = static_cast<int>(value);
    else if (paramId == "velocityB")
        velocityB_ = static_cast<int>(value);
    else if (paramId == "deviationRangeB")
        deviationRangeB_ = static_cast<int>(value);
    else if (paramId == "deviationIsBipolarB")
        deviationIsBipolarB_ = (value > 0.5f);

    // Global parameters
    else if (paramId == "channel")
        channel_ = static_cast<int>(value);
    else if (paramId == "rate")
        rate_ = value;
    else if (paramId == "noteProbability")
        noteProbability_ = value;
    else if (paramId == "durationBias")
        durationBias_ = value;

    if (patternChanged)
    {
        updatePatterns();
    }
}

float DualEuclideanGenerator::getParameter(const juce::String& paramId) const
{
    // Machine A parameters
    if (paramId == "stepsA")
        return static_cast<float>(stepsA_);
    else if (paramId == "pulsesA")
        return static_cast<float>(pulsesA_);
    else if (paramId == "noteA")
        return static_cast<float>(noteA_);
    else if (paramId == "velocityA")
        return static_cast<float>(velocityA_);
    else if (paramId == "deviationRangeA")
        return static_cast<float>(deviationRangeA_);
    else if (paramId == "deviationIsBipolarA")
        return deviationIsBipolarA_ ? 1.0f : 0.0f;

    // Machine B parameters
    else if (paramId == "stepsB")
        return static_cast<float>(stepsB_);
    else if (paramId == "pulsesB")
        return static_cast<float>(pulsesB_);
    else if (paramId == "noteB")
        return static_cast<float>(noteB_);
    else if (paramId == "velocityB")
        return static_cast<float>(velocityB_);
    else if (paramId == "deviationRangeB")
        return static_cast<float>(deviationRangeB_);
    else if (paramId == "deviationIsBipolarB")
        return deviationIsBipolarB_ ? 1.0f : 0.0f;

    // Global parameters
    else if (paramId == "channel")
        return static_cast<float>(channel_);
    else if (paramId == "rate")
        return rate_;
    else if (paramId == "noteProbability")
        return noteProbability_;
    else if (paramId == "durationBias")
        return durationBias_;

    return 0.0f;
}

void DualEuclideanGenerator::updatePatterns()
{
    // Обновляем паттерн для Machine A
    stepsA_ = juce::jmax(1, stepsA_);
    pulsesA_ = juce::jmax(0, juce::jmin(pulsesA_, stepsA_));
    patternA_.assign(stepsA_, false);
    if (pulsesA_ > 0)
    {
        int bucket = 0;
        for (int i = 0; i < stepsA_; ++i)
        {
            bucket += pulsesA_;
            if (bucket >= stepsA_)
            {
                bucket -= stepsA_;
                patternA_[i] = true;
            }
        }
    }

    // Обновляем паттерн для Machine B
    stepsB_ = juce::jmax(1, stepsB_);
    pulsesB_ = juce::jmax(0, juce::jmin(pulsesB_, stepsB_));
    patternB_.assign(stepsB_, false);
    if (pulsesB_ > 0)
    {
        int bucket = 0;
        for (int i = 0; i < stepsB_; ++i)
        {
            bucket += pulsesB_;
            if (bucket >= stepsB_)
            {
                bucket -= stepsB_;
                patternB_[i] = true;
            }
        }
    }

    // Сбрасываем мастер-счетчик
    masterStep_ = -1;
}

std::vector<int> DualEuclideanGenerator::getNotesInRange() const
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

int DualEuclideanGenerator::getNoteForMachine(int baseNote, int deviationRange, bool isBipolar, juce::Random& random) const
{
    if (deviationRange <= 0)
    {
        return baseNote;
    }

    auto availableNotes = getNotesInRange();
    if (availableNotes.empty())
    {
        return baseNote;
    }

    int minNote = isBipolar ? (baseNote - deviationRange) : baseNote;
    int maxNote = baseNote + deviationRange;

    // Фильтруем доступные ноты по диапазону отклонения
    std::vector<int> possibleNotes;
    for (int note : availableNotes)
    {
        if (note >= minNote && note <= maxNote)
        {
            possibleNotes.push_back(note);
        }
    }

    if (possibleNotes.empty())
    {
        return baseNote;
    }

    // Выбираем случайную ноту из возможных
    int randomIndex = random.nextInt(static_cast<int>(possibleNotes.size()));
    return possibleNotes[static_cast<size_t>(randomIndex)];
}