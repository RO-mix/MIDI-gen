# Гайд по портированию MIDI генератора на JUCE

## Введение

Этот документ описывает пошаговый процесс портирования MIDI генератора с Python на C++ с использованием фреймворка JUCE для создания VST плагина.

## Подготовка среды разработки

### Установка Visual Studio
1. Скачайте и установите Visual Studio Community (бесплатная версия)
2. Убедитесь, что установлен компилятор C++ (MSVC)
3. Установите Windows SDK

### Установка JUCE
1. Скачайте JUCE с официального сайта (https://juce.com/)
2. Распакуйте архив в удобное место (например, `C:\JUCE`)
3. Запустите Projucer (инструмент для управления проектами JUCE)

## Создание проекта VST плагина

### Настройка проекта в Projucer
1. Откройте Projucer
2. Создайте новый проект: `File → New Project`
3. Выберите тип проекта: `Audio Plug-in`
4. Задайте параметры проекта:
   - Project Name: `CreativeMIDIGenerator`
   - Project Type: `VST3 Plugin`
   - Plugin Name: `Creative MIDI Generator`
   - Plugin Description: `MIDI Pattern Generator with Euclidean rhythms`
   - Plugin Manufacturer: `Creative MIDI`
   - Plugin Code: `CrMG` (4-символьный код)
   - Plugin Channel Configurations: `No audio ports` (только MIDI)
   - Plugin MIDI Input/Output: `Yes`

5. Настройте пути к JUCE:
   - В меню `Paths` укажите путь к установленному JUCE

6. Сохраните проект: `File → Save Project`

### Генерация проекта для Visual Studio
1. В Projucer выберите `Save and Open in IDE`
2. Projucer автоматически сгенерирует проект Visual Studio

## Структура проекта JUCE

```
CreativeMIDIGenerator/
├── Source/
│   ├── PluginProcessor.h/cpp      # Основной класс плагина
│   ├── PluginEditor.h/cpp         # Класс редактора (UI)
│   ├── Generators/                # Алгоритмы генерации
│   │   ├── BaseGenerator.h/cpp
│   │   ├── RandomGenerator.h/cpp
│   │   ├── EuclideanGenerator.h/cpp
│   │   ├── DualEuclideanGenerator.h/cpp
│   │   └── RandomGeneratorV2.h/cpp
│   ├── Sequencer/                 # Секвенсор
│   │   ├── Sequencer.h/cpp
│   │   └── MidiHandler.h/cpp
│   ├── Theory/                    # Музыкальная теория
│   │   ├── Scales.h/cpp
│   │   └── Durations.h/cpp
│   ├── Looper/                    # Looper
│   │   └── Looper.h/cpp
│   └── UI/                        # Компоненты интерфейса
│       ├── PatternVisualizer.h/cpp
│       ├── ControlPanel.h/cpp
│       └── PresetManager.h/cpp
├── Resources/                     # Ресурсы (шрифты, изображения)
└── Tests/                         # Тесты
```

## Портирование компонентов

### 1. Алгоритмы генерации

#### BaseGenerator
Создайте абстрактный базовый класс для всех генераторов:

```cpp
// BaseGenerator.h
#pragma once
#include <JuceHeader.h>

class BaseGenerator
{
public:
    virtual ~BaseGenerator() = default;
    
    virtual std::pair<std::vector<std::pair<juce::MidiMessage, double>>, double> 
    generate(double currentBeat) = 0;
    
    virtual void setParameter(const juce::String& paramId, float value) = 0;
    virtual float getParameter(const juce::String& paramId) const = 0;
};
```

#### RandomGenerator
Портируйте логику случайной генерации нот:

```cpp
// RandomGenerator.h
#pragma once
#include "BaseGenerator.h"

class RandomGenerator : public BaseGenerator
{
public:
    std::pair<std::vector<std::pair<juce::MidiMessage, double>>, double> 
    generate(double currentBeat) override;
    
    void setParameter(const juce::String& paramId, float value) override;
    float getParameter(const juce::String& paramId) const override;

private:
    int midiChannel = 0;
    int octaveRange = 3;
    float density = 0.5f;
    int velocity = 100;
    // Другие параметры...
};
```

#### EuclideanGenerator
Реализуйте алгоритм евклидовых ритмов:

```cpp
// EuclideanGenerator.h
#pragma once
#include "BaseGenerator.h"

class EuclideanGenerator : public BaseGenerator
{
public:
    std::pair<std::vector<std::pair<juce::MidiMessage, double>>, double> 
    generate(double currentBeat) override;
    
    void setParameter(const juce::String& paramId, float value) override;
    float getParameter(const juce::String& paramId) const override;

private:
    // Параметры евклидова генератора
    int steps = 16;
    int beats = 4;
    int rotation = 0;
    int note = 60;
    // ...
};
```

Аналогично портируйте DualEuclideanGenerator и RandomGeneratorV2.

### 2. Секвенсор

Создайте класс секвенсора, адаптированный для VST:

```cpp
// Sequencer.h
#pragma once
#include <JuceHeader.h>
#include "Generators/BaseGenerator.h"

class Sequencer
{
public:
    Sequencer();
    ~Sequencer();
    
    void setGenerator(std::unique_ptr<BaseGenerator> gen);
    void setTempo(double bpm);
    void setPlaying(bool playing);
    
    void processBlock(juce::MidiBuffer& midiMessages, int numSamples);
    
private:
    std::unique_ptr<BaseGenerator> generator;
    double tempo = 120.0;
    bool isPlaying = false;
    double currentBeat = 0.0;
    double samplesPerBeat = 0.0;
    int sampleRate = 44100;
    
    void updateTiming();
};
```

### 3. Музыкальная теория

Портируйте классы для работы с ладами и длительностями:

```cpp
// Scales.h
#pragma once
#include <JuceHeader.h>

class Scales
{
public:
    static const std::map<juce::String, std::vector<int>> scaleIntervals;
    
    static bool isNoteInScale(int note, const juce::String& scaleName);
    static std::vector<int> getScaleNotes(int rootNote, const juce::String& scaleName);
    
private:
    static std::vector<int> major;
    static std::vector<int> minor;
    // Другие лады...
};
```

### 4. Looper

Реализуйте функциональность looper'а:

```cpp
// Looper.h
#pragma once
#include <JuceHeader.h>

class Looper
{
public:
    void recordNote(const juce::MidiMessage& message, double beatTime);
    void startPlayback();
    void stopPlayback();
    void clear();
    
    juce::MidiBuffer getPlaybackBuffer(int numSamples, double startTime, double endTime);
    
private:
    struct RecordedNote {
        juce::MidiMessage message;
        double beatTime;
    };
    
    std::vector<RecordedNote> recordedNotes;
    bool isRecording = false;
    bool isPlaying = false;
    double loopStart = 0.0;
    double loopEnd = 4.0; // 4 beats
};
```

### 5. Пользовательский интерфейс

Создайте UI компоненты в JUCE:

```cpp
// PluginEditor.h
#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class CreativeMidiGeneratorAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    CreativeMidiGeneratorAudioProcessorEditor (CreativeMidiGeneratorAudioProcessor&);
    ~CreativeMidiGeneratorAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    CreativeMidiGeneratorAudioProcessor& audioProcessor;
    
    // UI компоненты
    juce::ComboBox generatorSelector;
    juce::Slider tempoSlider;
    juce::Slider densitySlider;
    juce::TextButton playButton;
    juce::TextButton recordButton;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CreativeMidiGeneratorAudioProcessorEditor)
};
```

## Интеграция компонентов

### PluginProcessor
Интегрируйте все компоненты в основной класс плагина:

```cpp
// PluginProcessor.cpp
#include "PluginProcessor.h"
#include "PluginEditor.h"

CreativeMidiGeneratorAudioProcessor::CreativeMidiGeneratorAudioProcessor()
     : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                     .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    // Инициализация компонентов
    sequencer = std::make_unique<Sequencer>();
    currentGenerator = std::make_unique<RandomGenerator>();
    sequencer->setGenerator(std::move(currentGenerator));
}

void CreativeMidiGeneratorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Очистка буфера (плагин только MIDI)
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Обработка MIDI через секвенсор
    sequencer->processBlock(midiMessages, buffer.getNumSamples());
}

juce::AudioProcessorParameter* CreativeMidiGeneratorAudioProcessor::getParameter(int index)
{
    // Реализация параметров для автоматизации в DAW
}
```

## Система параметров и пресетов

### Параметры плагина
Реализуйте параметры для автоматизации в DAW:

```cpp
// В PluginProcessor.h
juce::AudioProcessorValueTreeState parameters;
std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> tempoAttachment;

// В PluginProcessor.cpp
juce::AudioProcessorValueTreeState::ParameterLayout 
CreativeMidiGeneratorAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "tempo", "Tempo", 60.0f, 200.0f, 120.0f));
        
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "density", "Density", 0.0f, 1.0f, 0.5f));
        
    // Другие параметры...
    
    return { params.begin(), params.end() };
}
```

### Пресеты
Реализуйте систему пресетов:

```cpp
// PresetManager.h
#pragma once
#include <JuceHeader.h>

class PresetManager
{
public:
    PresetManager(juce::AudioProcessorValueTreeState& state);
    
    void savePreset(const juce::String& name);
    void loadPreset(const juce::String& name);
    void deletePreset(const juce::String& name);
    
    juce::StringArray getPresetNames() const;
    
private:
    juce::AudioProcessorValueTreeState& valueTreeState;
    juce::File presetDirectory;
    
    void createPresetDirectory();
};
```

## Тестирование

### Unit тесты
Создайте тесты для проверки корректности алгоритмов:

```cpp
// Tests/GeneratorTests.cpp
#include <JuceHeader.h>
#include "../Source/Generators/RandomGenerator.h"

class RandomGeneratorTests : public juce::UnitTest
{
public:
    RandomGeneratorTests() : juce::UnitTest("RandomGenerator Tests") {}

    void runTest() override
    {
        beginTest("Basic generation");
        
        RandomGenerator generator;
        auto result = generator.generate(0.0);
        
        expect(result.first.size() > 0, "Should generate at least one event");
        
        // Другие тесты...
    }
};

static RandomGeneratorTests randomGeneratorTests;
```

### Тестирование в DAW
Протестируйте плагин в различных DAW:
1. Ableton Live
2. FL Studio
3. Reaper
4. Cubase
5. Logic Pro (macOS)

## Оптимизация

### Производительность
1. Минимизируйте аллокации памяти в audio thread
2. Используйте lock-free структуры данных для межпоточного взаимодействия
3. Оптимизируйте алгоритмы генерации

### Совместимость
1. Проверьте работу в разных версиях VST
2. Обеспечьте обратную совместимость пресетов
3. Тестируйте на разных операционных системах

## Сборка и дистрибуция

### Сборка для разных платформ
1. Windows: Сборка через Visual Studio
2. macOS: Сборка через Xcode
3. Linux: Сборка через Makefile или CMake

### Установка
1. Создайте инсталлятор для каждой платформы
2. Подпишите плагины (для macOS)
3. Создайте документацию для пользователей

## Заключение

Портирование MIDI генератора на JUCE требует значительных усилий, но обеспечивает кроссплатформенную совместимость и интеграцию с большинством DAW. Следуя этому гайду, вы сможете создать полнофункциональный VST плагин с сохранением всех ключевых возможностей оригинального приложения.