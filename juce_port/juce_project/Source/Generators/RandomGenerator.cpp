#include "RandomGenerator.h"
#include "Duration.h"

RandomGenerator::RandomGenerator()
{
    // Инициализация генератора случайных чисел
    random_.setSeed(juce::Time::currentTimeMillis());
    
    // Инициализация параметров по умолчанию через GenerationParameters
    params_.minNote = 48;  // C3
    params_.maxNote = 72;  // C5
    params_.velocityMin = 40.0f / 127.0f; // Нормализуем до 0-1
    params_.velocityMax = 100.0f / 127.0f; // Нормализуем до 0-1
    params_.noteDensity = 0.5f;
    params_.noteLength = 1.0f;
    params_.durationBias = 0.5f;
    params_.rootNote = 60; // C4
    params_.scaleType = ScaleType::Major; // Убираем приведение к int
    params_.rhythmComplexity = 0.5f;
    params_.humanization = 0.2f;
    params_.octaveRange = 2;
    params_.useChordProgression = false;
    params_.addCC74 = false;
    
    // Инициализация масштаба
    updateScaleNotes();
    
    // Инициализация аккордовой прогрессии по умолчанию
    chordProgression_[0] = 0; // I
    chordProgression_[1] = 1; // ii
    chordProgression_[2] = 2; // iii
    chordProgression_[3] = 3; // IV
    
    // Инициализация новых параметров
    swingAmount_ = 0.0f;
    accentIntensity_ = 0.5f;
    grooveTemplate_ = 0;
    microTiming_ = 0.0f;
    probabilityCurve_ = 0.5f;
    velocityResponse_ = 0.5f;
    noteLengthVariation_ = 0.0f;
    
    // Параметры арпеджио
    arpMode_ = 0;
    arpDirection_ = 0;
    arpOctaves_ = 1;
    arpRate_ = 0.5f;
    
    // Параметры аккордов
    chordVoicing_ = 0;
    chordInversions_ = 0;
    strumSpeed_ = 0.5f;
    strumDirection_ = 0;
}

std::pair<std::vector<std::pair<juce::MidiMessage, double>>, double>
RandomGenerator::generate(double currentBeat)
{
    std::vector<std::pair<juce::MidiMessage, double>> events;

    // Проверяем вероятность генерации ноты с учетом сложности ритма
    float effectiveProbability = params_.noteDensity;
    if (params_.rhythmComplexity > 0.5f) {
        // При высокой сложности ритма уменьшаем вероятность для создания более разреженного ритма
        effectiveProbability *= (1.0f - (params_.rhythmComplexity - 0.5f) * 0.5f);
    }
    
    if (random_.nextFloat() > effectiveProbability)
    {
        float rhythmVariation = getRhythmVariation();
        return {events, rhythmVariation}; // Нет ноты, используем вариативный ритм
    }

    // Получаем доступные ноты (в масштабе или аккорде)
    auto availableNotes = params_.useChordProgression ? getNotesInChord() : getNotesInRange();
    if (availableNotes.empty())
    {
        float rhythmVariation = getRhythmVariation();
        return {events, rhythmVariation};
    }

    // Выбираем случайную ноту с учетом человечности и сложности ритма
    int randomIndex = random_.nextInt(availableNotes.size());
    int randomNote = applyHumanization(availableNotes[randomIndex]);

    // Вычисляем динамику с учетом новых параметров и человечности
    int baseVelocity = random_.nextInt(params_.velocityMax - params_.velocityMin + 1) + params_.velocityMin;
    int randomVelocity = baseVelocity;
    
    if (params_.humanization > 0.0f) {
        int velocityVariation = static_cast<int>((random_.nextFloat() - 0.5f) * params_.humanization * 20.0f);
        randomVelocity = juce::jlimit(1, 127, randomVelocity + velocityVariation);
    }

    // Создаем MIDI сообщение note_on
    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(channel_, randomNote, static_cast<uint8>(randomVelocity));
    
    // Вычисляем длительность с учетом bias и сложности ритма
    float baseDuration = getRandomDuration();
    float duration = baseDuration;
    
    // Применяем влияние сложности ритма на длительность
    if (params_.rhythmComplexity > 0.5f) {
        // При высокой сложности делаем длительности более вариативными
        float durationVariation = (random_.nextFloat() - 0.5f) * params_.rhythmComplexity * 0.5f;
        duration = juce::jlimit(0.1f, 2.0f, duration * (1.0f + durationVariation));
    }

    events.push_back({noteOn, duration});

    // Опционально добавляем CC74 (brightness) с человечностью
    if (params_.addCC74)
    {
        int ccValue = random_.nextInt(128);
        if (params_.humanization > 0.0f) {
            int ccVariation = static_cast<int>((random_.nextFloat() - 0.5f) * params_.humanization * 30.0f);
            ccValue = juce::jlimit(0, 127, ccValue + ccVariation);
        }
        juce::MidiMessage cc74 = juce::MidiMessage::controllerEvent(channel_, 74, ccValue);
        events.push_back({cc74, 0.0}); // CC сообщения не имеют длительности
    }

    // Обновляем индекс аккорда для прогрессии
    if (params_.useChordProgression) {
        currentChordIndex_ = (currentChordIndex_ + 1) % 4;
    }

    float rhythmVariation = getRhythmVariation();
    return {events, rhythmVariation};
}

void RandomGenerator::setParameter(const juce::String& paramId, float value)
{
    if (paramId == "minNote")
        minNote_ = static_cast<int>(value);
    else if (paramId == "maxNote")
        maxNote_ = static_cast<int>(value);
    else if (paramId == "velocityMin")
        velocityMin_ = static_cast<int>(value);
    else if (paramId == "velocityMax")
        velocityMax_ = static_cast<int>(value);
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
    else if (paramId == "durationBias")
        durationBias_ = value;
    else if (paramId == "scaleRoot")
    {
        scaleRoot_ = static_cast<int>(value);
        updateScaleNotes();
    }
    else if (paramId == "scaleType")
    {
        // Предполагаем, что value соответствует enum ScaleType
        params_.scaleType = static_cast<ScaleType>(static_cast<int>(value));
        updateScaleNotes();
    }
    else if (paramId == "octaveRange")
    {
        octaveRange_ = static_cast<int>(value);
        updateScaleNotes();
    }
    else if (paramId == "rhythmComplexity")
        rhythmComplexity_ = value;
    else if (paramId == "humanization")
        humanization_ = value;
    else if (paramId == "useChordProgression")
        useChordProgression_ = (value > 0.5f);
    else if (paramId == "swingAmount")
        swingAmount_ = value;
    else if (paramId == "accentIntensity")
        accentIntensity_ = value;
    else if (paramId == "grooveTemplate")
        grooveTemplate_ = static_cast<int>(value);
    else if (paramId == "microTiming")
        microTiming_ = value;
    else if (paramId == "probabilityCurve")
        probabilityCurve_ = value;
    else if (paramId == "velocityResponse")
        velocityResponse_ = value;
    else if (paramId == "noteLengthVariation")
        noteLengthVariation_ = value;
    else if (paramId == "arpMode")
        arpMode_ = static_cast<int>(value);
    else if (paramId == "arpDirection")
        arpDirection_ = static_cast<int>(value);
    else if (paramId == "arpOctaves")
        arpOctaves_ = static_cast<int>(value);
    else if (paramId == "arpRate")
        arpRate_ = value;
    else if (paramId == "chordVoicing")
        chordVoicing_ = static_cast<int>(value);
    else if (paramId == "chordInversions")
        chordInversions_ = static_cast<int>(value);
    else if (paramId == "strumSpeed")
        strumSpeed_ = value;
    else if (paramId == "strumDirection")
        strumDirection_ = static_cast<int>(value);
}

float RandomGenerator::getParameter(const juce::String& paramId) const
{
    if (paramId == "minNote")
        return static_cast<float>(minNote_);
    else if (paramId == "maxNote")
        return static_cast<float>(maxNote_);
    else if (paramId == "velocityMin")
        return static_cast<float>(velocityMin_) / 127.0f;
    else if (paramId == "velocityMax")
        return static_cast<float>(velocityMax_) / 127.0f;
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
    else if (paramId == "durationBias")
        return durationBias_;
    else if (paramId == "scaleRoot")
        return static_cast<float>(scaleRoot_);
    else if (paramId == "scaleType")
        return static_cast<float>(static_cast<int>(params_.scaleType));
    else if (paramId == "octaveRange")
        return static_cast<float>(octaveRange_);
    else if (paramId == "rhythmComplexity")
        return rhythmComplexity_;
    else if (paramId == "humanization")
        return humanization_;
    else if (paramId == "useChordProgression")
        return useChordProgression_ ? 1.0f : 0.0f;
    else if (paramId == "swingAmount")
        return swingAmount_;
    else if (paramId == "accentIntensity")
        return accentIntensity_;
    else if (paramId == "grooveTemplate")
        return static_cast<float>(grooveTemplate_);
    else if (paramId == "microTiming")
        return microTiming_;
    else if (paramId == "probabilityCurve")
        return probabilityCurve_;
    else if (paramId == "velocityResponse")
        return velocityResponse_;
    else if (paramId == "noteLengthVariation")
        return noteLengthVariation_;
    else if (paramId == "arpMode")
        return static_cast<float>(arpMode_);
    else if (paramId == "arpDirection")
        return static_cast<float>(arpDirection_);
    else if (paramId == "arpOctaves")
        return static_cast<float>(arpOctaves_);
    else if (paramId == "arpRate")
        return arpRate_;
    else if (paramId == "chordVoicing")
        return static_cast<float>(chordVoicing_);
    else if (paramId == "chordInversions")
        return static_cast<float>(chordInversions_);
    else if (paramId == "strumSpeed")
        return strumSpeed_;
    else if (paramId == "strumDirection")
        return static_cast<float>(strumDirection_);

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

    // Более точная аппроксимация бета-распределения
    float u1 = random_.nextFloat();
    float u2 = random_.nextFloat();

    float x = 0.0f;
    if (alpha == 1.0f && beta_param == 1.0f)
    {
        x = u1; // Равномерное распределение
    }
    else
    {
        // Используем метод инверсии для бета-распределения
        // Это более точная аппроксимация чем предыдущая версия
        x = std::pow(u1, 1.0f / alpha) / (std::pow(u1, 1.0f / alpha) + std::pow(u2, 1.0f / beta_param));
    }

    int velocity = static_cast<int>(velocityMin_ + x * (velocityMax_ - velocityMin_));
    return juce::jlimit(velocityMin_, velocityMax_, velocity);
}

float RandomGenerator::getRandomDuration() const
{
    // Используем новый Duration модуль для вероятностной генерации длительностей
    return Duration::getProbabilisticDuration(durationBias_, random_);
}

void RandomGenerator::setGenerationParameters(const GenerationParameters& params)
{
    // Синхронизируем параметры с GenerationParameters
    params_ = params;
    
    // Устанавливаем параметры генератора из структуры
    minNote_ = params.minNote;
    maxNote_ = params.maxNote;
    velocityMin_ = static_cast<int>(params.velocityMin * 127.0f);
    velocityMax_ = static_cast<int>(params.velocityMax * 127.0f);
    noteProbability_ = params.noteDensity;
    rate_ = params.noteLength;
    
    // Рассчитываем velocityBias на основе диапазона velocity
    float velocityRange = params.velocityMax - params.velocityMin;
    velocityBias_ = velocityRange > 0.0f ? (params.velocityMax - 0.5f * velocityRange) : 0.5f;
    
    durationBias_ = params.durationBias;
    rhythmComplexity_ = params.rhythmComplexity;
    humanization_ = params.humanization;
    octaveRange_ = params.octaveRange;
    useChordProgression_ = params.useChordProgression;
    addCC74_ = params.addCC74;
    
    // Устанавливаем масштаб
    scaleRoot_ = params.rootNote;
    scaleType_ = static_cast<int>(params.scaleType); // Это может быть устаревшим, но оставим для совместимости
    
    // Копируем прогрессию аккордов
    for (int i = 0; i < 4; ++i) {
        chordProgression_[i] = params.chordProgression[i];
    }
    
    // Устанавливаем сид для генератора случайных чисел
    if (params.seed != 0) {
        random_.setSeed(params.seed);
    }
    
    updateScaleNotes();
}

void RandomGenerator::updateScaleNotes()
{
    // Получаем ноты выбранного масштаба
    scaleNotes_ = params_.getScaleNotes(); // Используем метод из новой структуры
    
    // Расширяем масштаб на заданный диапазон октав
    std::vector<int> extendedNotes;
    for (int octave = 0; octave < octaveRange_; ++octave) {
        for (int note : scaleNotes_) {
            extendedNotes.push_back(note + octave * 12);
        }
    }
    scaleNotes_ = extendedNotes;
}

std::vector<int> RandomGenerator::getNotesInChord() const
{
    if (!useChordProgression_) {
        return getNotesInRange();
    }
    
    // Получаем ноты текущего аккорда
    int chordDegree = chordProgression_[currentChordIndex_];
    std::vector<int> chordNotes = Scales::getChordNotes(scaleRoot_,
                                                       params_.scaleType, // scaleType теперь ScaleType
                                                       chordDegree);
    
    // Фильтруем по диапазону
    std::vector<int> filteredNotes;
    for (int note : chordNotes) {
        for (int octave = 0; octave < octaveRange_; ++octave) {
            int transposedNote = note + octave * 12;
            if (transposedNote >= minNote_ && transposedNote <= maxNote_) {
                filteredNotes.push_back(transposedNote);
            }
        }
    }
    
    return filteredNotes;
}

float RandomGenerator::getRhythmVariation() const
{
    // Используем rhythmComplexity для вариации ритма
    float baseRate = params_.noteLength;
    float variation = (random_.nextFloat() - 0.5f) * params_.rhythmComplexity * 0.5f;
    return baseRate * (1.0f + variation);
}

int RandomGenerator::applyHumanization(int note) const
{
    if (params_.humanization <= 0.0f) {
        return note;
    }
    
    // Добавляем случайное отклонение для "человечности"
    int deviation = static_cast<int>((random_.nextFloat() - 0.5f) * params_.humanization * 4.0f);
    return juce::jlimit(0, 127, note + deviation);
}