# AGENTS.md - Руководство по разработке Creative MIDI Generator (JUCE Port)

## 🎯 Философия разработки

### Headless-First подход
Проект использует **headless-first** методологию разработки, где основное внимание уделяется логике и автоматизированному тестированию без GUI зависимостей.

**Преимущества:**
- 🚀 **Быстрая разработка** - тестирование без GUI накладных расходов
- 🔧 **Автоматизация** - полная интеграция с CI/CD
- 🐛 **Раннее обнаружение ошибок** - тесты на уровне логики
- 📊 **Метрики качества** - измеримое покрытие тестами
- 🎵 **Фокус на музыке** - приоритет функциональности над интерфейсом

### Двухуровневая архитектура
```
┌─────────────────┐    ┌─────────────────┐
│  Standalone App │    │   VST Plugin    │
│   (Тестирование)│    │   (Продакшен)   │
│                 │    │                 │
│ ┌─────────────┐ │    │ ┌─────────────┐ │
│ │ Headless    │ │    │ │ GUI         │ │
│ │ Core        │◄┼──┼──►│ Interface   │ │
│ │ Logic       │ │    │ │             │ │
│ └─────────────┘ │    │ └─────────────┘ │
└─────────────────┘    └─────────────────┘
```

## 🧪 Система тестирования

### HeadlessTester - автоматизированные тесты
Расположен в `juce_project/Source/Tests/HeadlessTester.h/cpp`

**Возможности:**
- ✅ Тестирование генераторов паттернов
- ✅ Валидация музыкальных ладов
- ✅ Статистический анализ распределения
- ✅ Проверка граничных условий
- ✅ Автоматическое логирование

### Типы тестов

#### 1. Unit тесты генераторов
```cpp
// Пример тестирования RandomGenerator
bool testRandomGeneratorBasic() {
    auto generator = std::make_unique<RandomGenerator>();
    auto [events, duration] = generator->generate(0.0);

    // Проверяем генерацию MIDI событий
    return !events.empty();
}
```

#### 2. Тесты музыкальных ладов
```cpp
// Валидация всех 19 поддерживаемых ладов
bool testScales() {
    Scales scales;
    StringArray scaleNames = scales.getAvailableScaleNames();

    for (const auto& scaleName : scaleNames) {
        auto notes = scales.getScaleNotes(60, scaleName);
        // Проверяем корректность интервалов
    }
}
```

#### 3. Статистические тесты
```cpp
// Проверка равномерности распределения
bool testRandomDistribution() {
    std::map<int, int> noteCounts;
    // Собираем статистику из 1000 генераций
    // Проверяем равномерность распределения
}
```

### Запуск тестов

#### Через standalone приложение
```bash
# Сборка и запуск headless test runner
./CreativeMIDIGenerator.exe --run-tests
```

#### Через Visual Studio
1. Открыть проект
2. Собрать конфигурацию "HeadlessTest"
3. Запустить `HeadlessTestRunner`

#### Через CMake
```bash
cd juce_project
cmake -B build -DJUCE_PATH=D:/PROG/JUCE
cmake --build build --target HeadlessTestRunner
./build/HeadlessTestRunner.exe
```

## 📊 Метрики качества

### Обязательные метрики
- **Покрытие тестами**: минимум 80%
- **Время сборки**: менее 2 минут
- **Время тестов**: менее 30 секунд
- **Количество тестов**: минимум 10 на генератор

### Автоматическая отчетность
```
=== HEADLESS TEST RESULTS ===
RandomGenerator Basic: PASSED
RandomGenerator Scales: PASSED
RandomGenerator Parameters: PASSED
Scales Basic: PASSED
Scales Intervals: PASSED
Random Distribution Test: PASSED
Scale Filtering Test: PASSED
Parameter Ranges Test: PASSED

SUMMARY: 8/8 tests passed
🎉 All tests passed!
```

## 🔄 CI/CD интеграция

### GitHub Actions workflow
```yaml
name: CI/CD Pipeline

on: [push, pull_request]

jobs:
  test:
    runs-on: windows-latest
    steps:
    - uses: actions/checkout@v2
    - name: Setup JUCE
      run: # Скачать и настроить JUCE
    - name: Build
      run: cmake --build build --config Release
    - name: Run Tests
      run: ./build/HeadlessTestRunner.exe
```

### Локальная проверка качества
```powershell
# Полная проверка перед коммитом
.\run_quality_check.ps1

# Включает:
# - Сборку всех конфигураций
# - Запуск всех тестов
# - Проверку покрытия
# - Статический анализ кода
```

## 🎵 Музыкальные требования

### Точность и стабильность
- **Тайминг**: точность 1мс или лучше
- **MIDI**: полная поддержка MIDI 1.0/2.0
- **Scales**: 19 музыкальных ладов
- **Generators**: модульная архитектура

### Производительность
- **Audio thread**: zero-allocation policy
- **Memory**: отсутствие утечек
- **CPU**: минимальная нагрузка
- **Real-time**: гарантированная работа

## 🏗️ Архитектурные принципы

### 1. SOLID принципы
- **Single Responsibility**: каждый генератор отвечает за один алгоритм
- **Open/Closed**: легко добавлять новые генераторы
- **Liskov Substitution**: все генераторы взаимозаменяемы
- **Interface Segregation**: минимальные интерфейсы
- **Dependency Inversion**: зависимости от абстракций

### 2. Модульность
```
BaseGenerator (interface)
├── RandomGenerator
├── EuclideanGenerator
├── DualEuclideanGenerator
└── RandomGeneratorV2
```

### 3. Независимость от платформы
- ✅ Windows (основная)
- ✅ macOS (дополнительно)
- ✅ Linux (дополнительно)

## 📝 Стандарты кодирования

### C++ стандарты
- **Версия**: C++17 минимум
- **Стиль**: JUCE coding standards
- **Документация**: Doxygen comments
- **Обработка ошибок**: exceptions + Result types

### Пример документации
```cpp
/**
 * @brief Генерирует случайную ноту в заданном диапазоне
 *
 * @param currentBeat Текущая позиция в такте
 * @return std::pair<std::vector<MidiEvent>, double>
 *         - first: сгенерированные MIDI события
 *         - second: длительность до следующего события
 *
 * @note Использует бета-распределение для динамики
 * @see Scales для работы с ладами
 */
std::pair<std::vector<std::pair<juce::MidiMessage, double>>, double>
RandomGenerator::generate(double currentBeat)
```

## 🚀 Развертывание

### Автоматическая сборка
```powershell
# Полная сборка всех форматов
.\build_all.ps1

# Генерирует:
# - VST3 плагин
# - Standalone приложение
# - Установщик
```

### Релизная проверка
- [ ] Все тесты проходят
- [ ] Сборка в Release режиме
- [ ] Проверка в DAW
- [ ] Проверка производительности
- [ ] Создание установщика

## 📈 Мониторинг и аналитика

### Метрики производительности
- Время генерации паттерна
- CPU использование
- Память (heap allocations)
- MIDI latency

### Логирование
```cpp
// Автоматическое логирование всех операций
Logger::writeToLog("RandomGenerator: Generated " + String(note) + " at beat " + String(beat));
```

### Отчеты
- Ежедневные отчеты по покрытию тестами
- Метрики производительности
- Анализ ошибок и исключений

## 🎯 Следующие шаги

### Этап 1: Завершение тестирования
- [ ] Реализовать все тесты для генераторов
- [ ] Настроить CI/CD pipeline
- [ ] Добавить performance тесты

### Этап 2: Расширение функциональности
- [ ] EuclideanGenerator с тестами
- [ ] Система пресетов
- [ ] MIDI Clock sync

### Этап 3: Пользовательский интерфейс
- [ ] GUI для standalone
- [ ] VST editor
- [ ] Визуализация паттернов

### Этап 4: Оптимизация и релиз
- [ ] Профилирование
- [ ] Оптимизация производительности
- [ ] Финальное тестирование

---

*Этот документ описывает JUCE версию проекта (VST3 порт)*