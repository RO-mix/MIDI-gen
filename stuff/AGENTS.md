# AGENTS.md - Руководство по разработке Creative MIDI Generator (Python)

## 🎯 Философия разработки

### Python-First подход
Проект использует **python-first** методологию разработки с фокусом на быструю разработку и тестирование музыкальной логики.

## 🏗️ Архитектура проекта

### Структура директорий
```
stuff/
├── src/                    # Основной код
│   ├── generators/        # Генераторы паттернов
│   ├── theory/           # Музыкальная теория
│   ├── engine/           # Движок секвенсора
│   ├── gui/              # Графический интерфейс
│   ├── looper/           # Looper функциональность
│   └── midi/             # MIDI обработка
├── tests/                # Автоматизированные тесты
├── docs/                 # Документация
└── presets/              # Пресеты пользователя
```

## 🧪 Система тестирования

### Тесты генераторов
- **test_random_generator.py** - тесты случайного генератора
- **test_euclidean_generator.py** - тесты евклидовых ритмов
- **test_dual_euclidean_generator.py** - тесты двойных евклидовых генераторов
- **test_scales.py** - тесты музыкальных ладов

### Запуск тестов
```bash
# Запуск всех тестов
pytest

# Запуск с покрытием
pytest --cov=./src --cov-report=html

# Запуск конкретного теста
pytest tests/test_random_generator.py
```

## 📊 Метрики качества

- **Покрытие тестами**: минимум 80%
- **Время выполнения тестов**: менее 30 секунд
- **Количество тестов**: минимум 20

## 🔄 CI/CD интеграция

### GitHub Actions
```yaml
name: Python CI
on: [push, pull_request]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v2
    - name: Set up Python
      uses: actions/setup-python@v2
      with:
        python-version: '3.8'
    - name: Install dependencies
      run: pip install -r requirements-dev.txt
    - name: Run tests
      run: pytest --cov=./src --cov-report=xml
    - name: Upload coverage
      uses: codecov/codecov-action@v2
```

## 🎵 Музыкальные компоненты

### Генераторы паттернов
1. **RandomGenerator** - случайные ноты с фильтрацией по ладам
2. **EuclideanGenerator** - классические евклидовы ритмы
3. **DualEuclideanGenerator** - двойные евклидовы генераторы
4. **RandomGeneratorV2** - улучшенная версия

### Музыкальная теория
- **14 музыкальных ладов** (Major, Minor, Dorian, Phrygian, etc.)
- **Система длительностей** (шестнадцатые, восьмые, четверти)
- **Поддержка октав** (C-1 до C10)

## 📝 Стандарты кодирования

### Python стандарты
- **Версия**: Python 3.8+
- **Стиль**: PEP 8
- **Документация**: Google-style docstrings
- **Типизация**: type hints для всех функций

### Пример документации
```python
def get_notes_in_scale(root_note: int, scale_name: str) -> List[int]:
    """
    Получить все ноты в заданном ладу.

    Args:
        root_note: Базовая нота лада (0-11)
        scale_name: Название лада

    Returns:
        Список MIDI номеров нот в ладу

    Raises:
        ValueError: Если лад не найден
    """
```

## 🚀 Развертывание

### Создание исполняемого файла
```bash
# С помощью PyInstaller
pyinstaller --onefile --windowed main.py

# С помощью cx_Freeze
python setup.py build
```

### Создание установщика
- **Windows**: Inno Setup
- **macOS**: create-dmg
- **Linux**: deb/rpm пакеты

## 📈 Мониторинг

### Метрики производительности
- Время генерации паттерна
- CPU использование
- Память (heap allocations)
- MIDI latency

### Логирование
```python
import logging
logging.info(f"Generated pattern with {len(notes)} notes")
```

## 🎯 Следующие этапы

### Этап 1: Оптимизация
- [ ] Профилирование производительности
- [ ] Оптимизация алгоритмов генерации
- [ ] Улучшение точности тайминга

### Этап 2: Расширение функциональности
- [ ] Новые типы генераторов
- [ ] Дополнительные лады
- [ ] Импорт/экспорт паттернов

### Этаp 3: Портирование на JUCE
- [ ] Создание VST3 плагина
- [ ] Реализация headless тестирования
- [ ] Кроссплатформенная поддержка

---

*Этот документ описывает Python версию проекта*