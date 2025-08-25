# Инструкция по компиляции Creative MIDI Generator с LooperModule

## Требования

### Для Windows с Visual Studio:
1. **Visual Studio 2022** (или 2019) с компонентами:
   - Desktop development with C++
   - Windows 10 SDK (или новее)
   - C++ CMake tools for Windows

2. **JUCE Framework** - должен быть установлен в папку `../../JUCE` относительно проекта

3. **CMake** - версия 3.15 или новее

## Шаги компиляции

### 1. Генерация проекта
```powershell
cd "путь\к\juce_port\juce_project"
cmake -B build -S . -G "Visual Studio 17 2022"  # или "Visual Studio 16 2019"
```

### 2. Открытие в Visual Studio
```powershell
# Открыть сгенерированный .sln файл
start build/CreativeMIDIGenerator.sln
```

### 3. Сборка в Visual Studio
1. Открыть решение в Visual Studio
2. Выбрать конфигурацию Release или Debug
3. Собрать цель `CreativeMIDIGenerator` (плагин)
4. Собрать цель `HeadlessTestRunner` (тесты)

### 4. Запуск тестов
1. Установить `HeadlessTestRunner` как startup project
2. Нажать F5 или запустить проект
3. Результаты тестов отобразятся в консоли

## Решение проблем

### Если генератор Visual Studio не найден:
```powershell
# Проверить установленные генераторы
cmake --help

# Попробовать другие генераторы
cmake -B build -S . -G "Ninja"
cmake -B build -S . -G "Unix Makefiles"
```

### Если JUCE не найден:
1. Убедитесь, что JUCE установлен в `../../JUCE` относительно проекта
2. Или измените путь в `CMakeLists.txt`:
```cmake
# Изменить эту строку на правильный путь к JUCE
add_subdirectory(../../JUCE JUCE)
```

## Структура проекта после реализации LooperModule

```
juce_project/
├── CMakeLists.txt                 # Основной файл сборки
├── Source/
│   ├── Generators/                # MIDI генераторы
│   │   ├── BaseGenerator.h/cpp
│   │   ├── RandomGenerator.h/cpp
│   │   ├── EuclideanGenerator.h/cpp
│   │   └── DualEuclideanGenerator.h/cpp
│   ├── Looper/                    # 🆕 НОВЫЙ: LooperModule
│   │   ├── Looper.h               # Интерфейс с двумя режимами
│   │   └── Looper.cpp             # Реализация всех функций
│   ├── Tests/                     # Тесты
│   │   ├── HeadlessTester.h/cpp   # 16 тестовых методов
│   │   └── HeadlessTestRunner.cpp
│   ├── PluginProcessor.h/cpp      # Интегрирован с Looper
│   └── PluginEditor.h/cpp         # GUI с элементами управления
├── build/                         # Сгенерированные файлы
└── COMPILATION_GUIDE.md           # Эта инструкция
```

## Функциональность LooperModule

### Режимы работы:
- **MIDI Looper**: классический лупер для паттернов
- **Generation Looper**: буферизация выходов генераторов

### Эффекты:
- Pitch Shift: ±12 полутонов
- Playback Speed: 0.25x - 4x
- Reverse: реверс воспроизведения

### GUI элементы:
- Кнопки Record/Play/Clear
- Переключатель режима работы
- Слайдеры эффектов
- Статусная строка

## Тестирование

Проект включает **16 тестовых методов**:
- 6 тестов генераторов
- 10 тестов LooperModule (включая 4 расширенных)
- 3 статистических теста

Все тесты запускаются через `HeadlessTestRunner`.