# Creative MIDI Generator - JUCE Port

Это новый этап разработки портирования Creative MIDI Generator на фреймворк JUCE для создания VST3 плагина.

## 🎯 Цели портирования

- Создать полнофункциональный VST3 плагин
- Сохранить всю функциональность оригинального Python приложения
- Обеспечить точность тайминга 1мс
- Интегрировать с основными DAW (Ableton Live, FL Studio, Reaper, Cubase)
- Создать современный пользовательский интерфейс

## 📁 Структура проекта

```
juce_port/
├── juce_project/                # Основной JUCE проект
│   ├── CreativeMIDIGenerator.jucer
│   ├── Source/
│   │   ├── PluginProcessor.h/cpp      # Основной класс плагина
│   │   ├── PluginEditor.h/cpp         # Класс редактора (UI)
│   │   ├── Generators/                # Алгоритмы генерации
│   │   │   ├── BaseGenerator.h/cpp
│   │   │   ├── RandomGenerator.h/cpp
│   │   │   ├── EuclideanGenerator.h/cpp
│   │   │   ├── DualEuclideanGenerator.h/cpp
│   │   │   └── RandomGeneratorV2.h/cpp
│   │   ├── Sequencer/                 # Секвенсор
│   │   │   ├── Sequencer.h/cpp
│   │   │   └── MidiHandler.h/cpp
│   │   ├── Theory/                    # Музыкальная теория
│   │   │   ├── Scales.h/cpp
│   │   │   └── Durations.h/cpp
│   │   ├── Looper/                    # Looper
│   │   │   └── Looper.h/cpp
│   │   └── UI/                        # Компоненты интерфейса
│   │       ├── PatternVisualizer.h/cpp
│   │       ├── ControlPanel.h/cpp
│   │       └── PresetManager.h/cpp
│   ├── Resources/                     # Ресурсы (шрифты, изображения)
│   └── Tests/                         # Тесты
├── docs/                          # Документация порта
├── tests/                         # Тесты порта
└── build_scripts/                 # Скрипты сборки
```

## 🚀 Начало работы

### Предварительные требования

1. **Visual Studio 2022** с компилятором C++17
2. **JUCE framework** установлен в `D:\PROG\JUCE`
3. **Projucer** (инструмент управления проектами JUCE)

### Настройка среды разработки

1. Откройте `juce_project/CreativeMIDIGenerator.jucer` в Projucer
2. Убедитесь, что пути к JUCE настроены правильно
3. Сгенерируйте проект для Visual Studio: `File → Save and Open in IDE`
4. Соберите проект в Visual Studio

## 📚 Документация

Основная документация находится в `../stuff/docs/`:
- [Гайд по портированию](../stuff/docs/port2juce_guide.md)
- [Документ передачи задачи](../stuff/docs/porting_handoff.md)
- [Техническая архитектура](../stuff/docs/technical-architecture.md)

## 🔄 Сравнение с оригиналом

Оригинальный Python прототип находится в `../stuff/` со следующей функциональностью:
- ✅ 4 типа генераторов паттернов (Random, Euclidean, Dual Euclidean, Random V2)
- ✅ Секвенсор с точностью 1мс
- ✅ 14 музыкальных ладов
- ✅ Looper (запись/воспроизведение)
- ✅ Система пресетов
- ✅ Многопоточная архитектура (GUI, Sequencer, MIDI)

## 🎵 Функциональные требования к порту

### Обязательные компоненты
- [ ] BaseGenerator - абстрактный базовый класс
- [ ] RandomGenerator - генератор случайных нот
- [ ] EuclideanGenerator - евклидовы ритмы
- [ ] DualEuclideanGenerator - двойные евклидовы ритмы
- [ ] RandomGeneratorV2 - улучшенный случайный генератор
- [ ] Sequencer - секвенсор с 1мс точностью
- [ ] Scales - система ладов (14 вариантов)
- [ ] Looper - запись и воспроизведение паттернов
- [ ] PresetManager - система сохранения/загрузки пресетов

### Пользовательский интерфейс
- [ ] PatternVisualizer - визуализация паттернов
- [ ] ControlPanel - панель управления параметрами
- [ ] PluginEditor - основной редактор плагина

## 🧪 Тестирование

Тесты для порта находятся в директории `tests/`:
- Unit-тесты для отдельных компонентов
- Интеграционные тесты
- Тестирование в различных DAW

## 🔧 Сборка и дистрибуция

### Целевые платформы
- **Windows** (VST3) - основная платформа
- **macOS** (VST3, AU) - дополнительно
- **Linux** (VST3) - дополнительно

### Системы сборки
- Visual Studio 2022 (Windows)
- Xcode (macOS)
- CMake (кроссплатформенно)

## 📋 План разработки

### Этап 1: Базовая инфраструктура (текущий)
- [x] Создание структуры директорий
- [x] Перемещение существующего проекта
- [x] Настройка базовой конфигурации
- [ ] Реализация BaseGenerator
- [ ] Реализация Scales

### Этап 2: Генераторы паттернов
- [ ] RandomGenerator
- [ ] EuclideanGenerator
- [ ] DualEuclideanGenerator
- [ ] RandomGeneratorV2

### Этап 3: Секвенсор и MIDI
- [ ] Sequencer с 1мс точностью
- [ ] MIDI обработка
- [ ] Синхронизация с DAW

### Этап 4: Looper и дополнительные функции
- [ ] Looper функциональность
- [ ] PresetManager
- [ ] Система параметров

### Этап 5: Пользовательский интерфейс
- [ ] PatternVisualizer
- [ ] ControlPanel
- [ ] PluginEditor

### Этап 6: Тестирование и оптимизация
- [ ] Unit-тесты
- [ ] Тестирование в DAW
- [ ] Оптимизация производительности
- [ ] Финальная отладка

## 🤝 Сотрудничество

Присоединяйтесь к разработке! Основные контакты:
- **Рам Джи** - ведущий разработчик
- **GitHub Issues** - для баг-репортов и предложений
- **Документация** - для понимания архитектуры

## 📄 Лицензия

Проект распространяется под лицензией MIT. См. файл LICENSE для деталей.

---

*Этот документ является частью нового этапа разработки Creative MIDI Generator*