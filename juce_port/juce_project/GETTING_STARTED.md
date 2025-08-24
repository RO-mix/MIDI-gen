# Начало работы с Creative MIDI Generator (JUCE Port)

## Содержание
1. [Требования](#требования)
2. [Установка JUCE](#установка-juce)
3. [Открытие проекта](#открытие-проекта)
4. [Сборка проекта](#сборка-проекта)
5. [Структура проекта](#структура-проекта)
6. [Добавление новых компонентов](#добавление-новых-компонентов)

## Требования

- JUCE Framework (версия 6.1.0 или выше)
- Visual Studio 2019 (для Windows)
- Xcode (для macOS)
- Компилятор C++ с поддержкой C++17

## Установка JUCE

1. Скачайте JUCE с официального сайта: https://juce.com/
2. Распакуйте архив в удобное место (например, `D:\PROG\JUCE`)
3. Запустите `Projucer.exe` из папки JUCE

## Открытие проекта

1. Запустите Projucer
2. Откройте файл `CreativeMIDIGenerator.jucer` из папки проекта
3. Убедитесь, что пути к JUCE настроены правильно:
   - Перейдите в `File → Global Paths`
   - Укажите путь к установленному JUCE

## Сборка проекта

### Windows (Visual Studio)

1. В Projucer нажмите `Save Project and Open in IDE`
2. Visual Studio автоматически откроется с проектом
3. Выберите конфигурацию (Debug/Release) и платформу (x64/x86)
4. Нажмите `Build → Build Solution`

### macOS (Xcode)

1. В Projucer нажмите `Save Project and Open in IDE`
2. Xcode автоматически откроется с проектом
3. Выберите целевую конфигурацию
4. Нажмите `Product → Build`

## Структура проекта

```
juce_project/
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
├── Tests/                         # Тесты
├── CreativeMIDIGenerator.jucer    # Файл проекта JUCE
├── README.md                      # Основная документация
└── GETTING_STARTED.md             # Этот файл
```

## Добавление новых компонентов

### Добавление нового генератора

1. Создайте новый файл в `Source/Generators/` (например, `MyNewGenerator.h/cpp`)
2. Унаследуйте класс от `BaseGenerator`
3. Реализуйте методы `generate()`, `setParameter()` и `getParameter()`
4. Добавьте файлы в проект через Projucer:
   - Откройте проект в Projucer
   - В разделе "Source" правой кнопкой мыши нажмите на папку "Generators"
   - Выберите "Add Existing Files..."
   - Выберите созданные файлы

### Добавление нового UI компонента

1. Создайте новый файл в `Source/UI/` (например, `MyNewComponent.h/cpp`)
2. Унаследуйте класс от `juce::Component`
3. Реализуйте необходимую функциональность
4. Добавьте файлы в проект через Projucer

## Полезные ссылки

- [Документация JUCE](https://juce.com/learn/documentation)
- [Форум поддержки JUCE](https://forum.juce.com/)
- [Исходный проект на Python](../stuff/)
- [Гайд по портированию](../stuff/docs/port2juce_guide.md)
- [Документ передачи задачи](../stuff/docs/porting_handoff.md)