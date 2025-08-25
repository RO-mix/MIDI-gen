# Архитектура MIDI лупера и лупизации генерации

## Два блока системы

### Блок 1: MIDI Лупер (MIDI Looper Generator)
**Назначение**: Классический MIDI лупер для записи и воспроизведения MIDI паттернов

**Функциональность**:
- Запись входящих MIDI событий в реальном времени
- Циклическое воспроизведение записанных паттернов
- Синхронизация по MIDI clock
- Многоканальная работа (16 MIDI каналов)

**Классы**:
- `MidiLooper` - основной класс лупера
- `LoopPattern` - паттерн с MIDI данными
- `LoopTrack` - отдельная дорожка лупера
- `MidiRecorder` - запись MIDI событий

### Блок 2: Лупизация генерации (Generation Looper Effect)
**Назначение**: Эффект буферизации для существующих генераторов

**Функциональность**:
- Захват вывода генераторов в буфер
- Циклическое воспроизведение буфера
- Реал-тайм манипуляции (reverse, speed, pitch)
- Слойная работа с несколькими буферами

**Классы**:
- `GenerationLooper` - эффект лупизации
- `BufferManager` - управление буферами
- `LooperEffect` - базовый класс эффектов
- `RealTimeProcessor` - обработка в реальном времени

## Интеграция с существующей системой

### Генераторы как источники данных
```python
# Каждый генератор может быть источником для лупера
class GeneratorWithLooper:
    def __init__(self, base_generator):
        self.generator = base_generator
        self.looper = GenerationLooper()
        self.midi_looper = MidiLooper()
    
    def process(self):
        # Получаем данные от генератора
        midi_data = self.generator.generate()
        
        # Отправляем в лупер генерации
        buffered = self.looper.process(midi_data)
        
        # Можем записать в MIDI лупер
        if self.recording_enabled:
            self.midi_looper.record(midi_data)
        
        return buffered
```

### Архитектура плагинов
```python
class LooperPlugin:
    """Базовый класс для луперов"""
    def __init__(self, name):
        self.name = name
        self.enabled = False
        self.parameters = {}
    
    def process(self, midi_events):
        raise NotImplementedError

class MidiLooperPlugin(LooperPlugin):
    """MIDI лупер как плагин"""
    def __init__(self):
        super().__init__("MIDI Looper")
        self.patterns = []
        self.current_pattern = None
    
    def process(self, midi_events):
        if self.current_pattern:
            return self.current_pattern.play()
        return midi_events

class GenerationLooperPlugin(LooperPlugin):
    """Лупизация генерации как плагин"""
    def __init__(self):
        super().__init__("Generation Looper")
        self.buffer_size = 4  # в барах
        self.buffer = []
    
    def process(self, midi_events):
        # Захватываем в буфер
        self.buffer.extend(midi_events)
        
        # Воспроизводим из буфера циклично
        if len(self.buffer) >= self.buffer_size:
            return self.buffer[:self.buffer_size]
        return midi_events
```

## Поток данных

### Схема работы
```
[MIDI Input] → [Генераторы] → [Generation Looper] → [MIDI Looper] → [MIDI Output]
                    ↓                    ↓                    ↓
              [Вариации]        [Буфер эффектов]    [Запись паттернов]
```

### Параллельная обработка
```python
class LooperSystem:
    def __init__(self):
        self.generators = []  # Существующие генераторы
        self.midi_looper = MidiLooper()
        self.gen_looper = GenerationLooper()
    
    def process_frame(self):
        # 1. Генерация от всех генераторов
        all_midi = []
        for gen in self.generators:
            midi = gen.generate()
            all_midi.extend(midi)
        
        # 2. Лупизация генерации
        looped = self.gen_looper.process(all_midi)
        
        # 3. Запись в MIDI лупер
        if self.midi_looper.recording:
            self.midi_looper.record(looped)
        
        # 4. Воспроизведение из лупера
        if self.midi_looper.playing:
            looped = self.midi_looper.play()
        
        return looped
```

## GUI интеграция

### Панель луперов
- **MIDI Looper Panel**: управление паттернами, запись, воспроизведение
- **Generation Looper Panel**: управление буферами, эффектами
- **Общая панель**: синхронизация, мастер-контроль

### Визуализация
- **Waveform view**: для Generation Looper
- **Piano roll**: для MIDI Looper
- **Buffer indicators**: состояние буферов
- **Sync indicators**: синхронизация между луперами

## Параметры и контроль

### MIDI Looper параметры
- Record length (1-32 bars)
- Loop mode (normal, reverse, ping-pong)
- Quantization (1/4, 1/8, 1/16, 1/32)
- Channel filtering (1-16)

### Generation Looper параметры
- Buffer size (1-8 bars)
- Playback speed (0.25x - 4x)
- Pitch shift (-12 to +12 semitones)
- Effects (reverse, stutter, freeze)

## Синхронизация

### MIDI Clock sync
```python
class LooperSync:
    def __init__(self):
        self.bpm = 120
        self.bar_position = 0
        self.beat_position = 0
    
    def sync_loopers(self, midi_looper, gen_looper):
        # Синхронизация позиций
        midi_looper.set_position(self.bar_position, self.beat_position)
        gen_looper.set_position(self.bar_position, self.beat_position)
```

### Автоматическая синхронизация
- Старт/стоп по MIDI clock
- Квантование к ближайшему бару
- Автоматическое выравнивание длины паттернов

