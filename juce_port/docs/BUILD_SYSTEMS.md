# Системы сборки Creative MIDI Generator

## Обзор

Проект использует две разные системы сборки для разных целей:

1. **CMake** - для хэдлесс тестов и CI/CD
2. **MSBuild (Visual Studio)** - для основного JUCE плагина

## CMake (Хэдлесс тесты)

### Назначение
- Автоматизированное тестирование логики генераторов
- CI/CD интеграция
- Быстрая проверка изменений без GUI

### Структура
```
tests/
├── CMakeLists.txt          # Основной CMake файл
├── src/
│   ├── test_main.cpp       # Точка входа для тестов
│   ├── test_generators.cpp # Тесты генераторов
│   ├── test_looper.cpp     # Тесты MIDI лупера
│   └── test_theory.cpp     # Тесты музыкальной теории
└── build/                  # Директория сборки
```

### Использование
```powershell
# Сборка и запуск тестов
cd tests
cmake -B build
cmake --build build
.\build\Debug\midi_tests.exe

# Или через скрипт
.\run_tests.ps1
```

### Особенности
- Использует Catch2 для тестирования
- Не требует JUCE для компиляции
- Быстрая сборка (~30 секунд)
- Покрытие только core логики

## MSBuild (JUCE плагин)

### Назначение
- Основной VST3/AU плагин
- GUI интерфейс
- Audio/MIDI обработка
- Релизная сборка

### Структура
```
juce_project/
├── CreativeMIDIGenerator.jucer  # JUCE проект
├── Source/                      # Исходный код
│   ├── PluginProcessor.cpp/h    # Основной процессор
│   ├── PluginEditor.cpp/h       # GUI редактор
│   ├── Generators/              # Генераторы MIDI
│   └── Theory/                  # Музыкальная теория
└── Builds/                      # Сгенерированные проекты
    └── VisualStudio2022/        # Visual Studio проект
```

### Использование
```powershell
# Через Projucer
1. Открыть CreativeMIDIGenerator.jucer
2. Выбрать Visual Studio 2022 exporter
3. File → Save and Open in IDE
4. Build → Build Solution

# Через скрипт
.\build_scripts\force_rebuild.ps1
```

### Особенности
- Требует JUCE framework
- Поддержка VST3/AU форматов
- GUI с JUCE компонентами
- Долгая сборка (~2-5 минут)
- Зависит от Windows SDK

## Сравнение систем

| Аспект | CMake (Тесты) | MSBuild (Плагин) |
|--------|---------------|------------------|
| **Скорость сборки** | ~30 сек | 2-5 мин |
| **Зависимости** | Catch2, C++17 | JUCE, Windows SDK |
| **Формат вывода** | .exe | .vst3, .dll, .exe |
| **GUI** | Нет | Да |
| **MIDI** | Эмуляция | Реальная обработка |
| **Цель** | Тестирование | Продакшен |
| **CI/CD** | Да | Ограничено |

## Рекомендации

### Для разработки
1. **Изменения в логике**: Сначала CMake тесты, затем MSBuild
2. **Изменения в GUI**: Только MSBuild
3. **Новые генераторы**: Начать с CMake тестов

### Для релиза
1. Убедиться что CMake тесты проходят
2. Собрать через MSBuild
3. Протестировать в DAW
4. Проверить все форматы (VST3/AU)

## Устранение проблем

### CMake тесты не собираются
```powershell
# Очистить кэш
Remove-Item -Recurse -Force tests/build
cmake -B tests/build
```

### MSBuild не находит JUCE
- Проверить путь: `D:\PROG\JUCE`
- Переэкспортировать проект из Projucer

### Конфликты имен
- CMake: `midi_tests.exe`
- MSBuild: `Creative MIDI Generator.vst3`
- Разные пространства имен предотвращают конфликты