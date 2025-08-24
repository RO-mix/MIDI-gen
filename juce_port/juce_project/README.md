# Creative MIDI Generator - JUCE Port

Это проект портирования Creative MIDI Generator на фреймворк JUCE для создания VST3 плагина.

## Структура проекта

Проект следует структуре, описанной в `stuff/docs/port2juce_guide.md`:

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
└── Tests/                         # Тесты
```

## Начало работы

1. Откройте проект в Projucer
2. Сгенерируйте проект для вашей IDE
3. Соберите проект

## Ссылки

- [Исходный проект на Python](../stuff/)
- [Гайд по портированию](../stuff/docs/port2juce_guide.md)
- [Документ передачи задачи](../stuff/docs/porting_handoff.md)