# Детальные спецификации Creative MIDI Looper & Pattern Studio

## 🎯 MIDI Looper Engine - Детальная спецификация

### Режимы записи
**Основной режим: Фиксированная длина (Вариант B)**

```python
class LoopRecordingMode:
    FIXED_LENGTH = "fixed_length"  # Основной режим
    MANUAL_TRIGGER = "manual_trigger"  # Альтернативный
    MIDI_TRIGGER = "midi_trigger"  # Для будущего
```

**Параметры записи:**
- Длина цикла: 2, 4, 8, 16 тактов (выбор через GUI)
- BPM: синхронизирован с DAW/внутренний
- Pre-count: 1 такт метронома перед записью
- Visual feedback: счетчик тактов + прогресс бар

**Процесс записи:**
1. Выбор длины цикла (dropdown)
2. Нажатие "Record" или клавиша "R"
3. Pre-count (1 такт метронома)
4. Запись в течение выбранной длины
5. Автоматическое переключение в режим "Loop"

## 🎵 Pattern Variation Engine - Детальная спецификация

### Типы вариаций (все включаются/выключаются отдельно)

#### 1. Velocity Variations
```python
class VelocityVariation:
    enabled: bool = True
    range_percent: int = 20  # ±20% от исходной громкости
    preserve_dynamics: bool = True  # сохранять относительные различия
```

#### 2. Timing Variations
```python
class TimingVariation:
    enabled: bool = True
    swing_amount: int = 0-100  # 0 = straight, 100 = full swing
    humanize_ms: float = 0-10  # случайные микро-сдвиги
```

#### 3. Density Variations
```python
class DensityVariation:
    enabled: bool = True
    probability_add: float = 0.1  # вероятность добавления ноты
    probability_remove: float = 0.1  # вероятность удаления
    min_density: float = 0.5  # минимальная плотность
```

#### 4. Harmonic Variations
```python
class HarmonicVariation:
    enabled: bool = True
    transpose_semitones: int = -12 to +12
    chord_extensions: list = ["7th", "9th", "sus2", "sus4"]
    scale_lock: bool = True  # оставаться в текущем ладу
```

## 🔄 Smart Loop Extension - Детальная спецификация

### Режимы эволюции
**Основной режим: Ручной контроль + MIDI CC**

```python
class EvolutionMode:
    MANUAL = "manual"  # Основной режим
    AUTOMATIC = "automatic"  # Каждые N циклов
    MIDI_CONTROLLED = "midi_controlled"  # CC параметр
```

**Параметры эволюции:**
- Evolution amount: 0-100% (CC #16)
- Evolution trigger: кнопка "Evolve" или MIDI нота C#3
- History depth: последние 10 версий паттерна
- Undo/Redo: Ctrl+Z / Ctrl+Y

## 🎹 Live Performance Mapping

### MIDI CC Mapping для Launchpad Pro + MIDI Keyboard
```python
MIDI_CC_MAPPING = {
    # Evolution Control
    16: "evolution_amount",  # Mod wheel
    17: "variation_intensity",  # Expression pedal
    
    # Variation Toggles
    20: "velocity_variation_on/off",
    21: "timing_variation_on/off", 
    22: "density_variation_on/off",
    23: "harmonic_variation_on/off",
    
    # Loop Control
    24: "record_start/stop",
    25: "clear_loop",
    26: "duplicate_loop",
    
    # Pattern Selection
    32: "pattern_slot_1",
    33: "pattern_slot_2",
    34: "pattern_slot_3",
    35: "pattern_slot_4"
}
```

### LED Feedback для Launchpad Pro
```python
LED_COLORS = {
    "recording": (255, 0, 0),      # Красный
    "playing": (0, 255, 0),        # Зеленый
    "variation_active": (0, 0, 255), # Синий
    "evolution_ready": (255, 255, 0) # Желтый
}
```

## 🧬 Pattern DNA System - Детальная структура

### DNA Формат
```json
{
  "pattern_id": "unique_uuid",
  "metadata": {
    "length_beats": 4,
    "bpm": 120,
    "scale": "C_minor",
    "created_at": "2025-08-15T14:00:00Z"
  },
  "dna": {
    "rhythm": {
      "grid": [1,0,1,1,0,1,0,1,1,0,1,0,1,1,0,1],
      "subdivision": 16
    },
    "velocity": {
      "values": [100,0,80,90,0,70,0,85,95,0,75,0,80,90,0,100],
      "curve": "linear"
    },
    "timing": {
      "micro_offsets": [0,0,-2,1,0,3,0,-1,2,0,-3,0,1,-2,0,0],
      "swing": 15
    },
    "harmony": {
      "root": "C",
      "mode": "minor",
      "chord_progression": ["Cm", "Ab", "Bb", "G"]
    }
  },
  "mutations": {
    "velocity_range": 20,
    "timing_humanize": 5,
    "density_factor": 0.8,
    "transpose_range": 7
  }
}
```

## 🎛️ GUI Layout - Детальная спецификация

### Main Window Layout
```
┌─────────────────────────────────────────────────────────┐
│  Transport Controls                    Pattern Slots    │
│  [Play] [Stop] [Record] [Tempo: 120] [Slot 1][2][3][4] │
├─────────────────────────────────────────────────────────┤
│  Loop Settings                    Variation Controls    │
│  Length: [4 beats ▼]            [☑] Velocity [Amount: ▓▓▓] │
│  Pre-count: [1 beat ▼]          [☑] Timing   [Swing: ▓▓▓] │
│  Metronome: [☑]                 [☑] Density  [Prob: ▓▓▓] │
├─────────────────────────────────────────────────────────┤
│  Pattern DNA Viewer                                     │
│  ┌─────────────────────────────────────────────────┐   │
│  │  [C-3] [D-3] [Eb-3] [F-3] [G-3] [Ab-3] [Bb-3] │   │
│  │  ▓▓░░▓▓▓░░▓░░▓▓▓░░▓░░▓▓▓░░▓░░▓▓▓░░▓░░▓▓▓░░▓▓ │   │
│  └─────────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────┤
│  Evolution Control                                      │
│  [Evolve Now] [Amount: ▓▓▓▓▓░░░░░] [Auto: □]          │
│  [Undo] [Redo] [Save DNA] [Load DNA]                    │
└─────────────────────────────────────────────────────────┘
```

## 🚀 Implementation Priority

### Phase 1: Core Looper (Q1 2025)
1. ✅ Fixed-length recording (4 beats default)
2. ✅ Basic loop playback
3. ✅ Visual metronome/count-in
4. ✅ Pattern slot system (4 slots)

### Phase 2: Variations (Q2 2025)
1. ✅ Velocity variations
2. ✅ Timing variations (swing)
3. ✅ Density variations
4. ✅ MIDI CC mapping

### Phase 3: Evolution (Q3 2025)
1. ✅ Manual evolution trigger
2. ✅ DNA save/load system
3. ✅ Undo/redo system
4. ✅ LED feedback

### Phase 4: Advanced (Q4 2025)
1. ✅ Harmonic variations
2. ✅ AI-assisted evolution
3. ✅ Live performance mode
4. ✅ Pattern sharing system

## 📋 Technical Requirements

### Dependencies
- python-rtmidi >= 1.4.9
- pygame >= 2.5.0
- numpy >= 1.24.0
- mido >= 1.2.10
- jsonschema >= 4.17.0

### Performance Targets
- MIDI latency: < 5ms
- GUI refresh rate: 60 FPS
- Pattern save/load: < 100ms
- Evolution calculation: < 50ms