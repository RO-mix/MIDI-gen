# Developer Documentation: Creative MIDI Generator

This document provides a comprehensive technical overview of the Creative MIDI Generator project, intended for developers and contributors.

## 1. Core Architecture

The application operates on a multi-threaded model to ensure a non-blocking, responsive user experience. The three main threads are:

1.  **Main Thread (GUI)**: Handles the user interface and user input, built with DearPyGui.
2.  **Sequencer Thread**: Manages time, generates musical patterns, and schedules MIDI events. This is the musical brain of the application.
3.  **MIDI Player Thread**: A dedicated, non-blocking thread for sending MIDI messages to the selected output device.

Communication between threads is handled by thread-safe queues (`queue.Queue`).

-   **`command_queue`**: The GUI places user commands (e.g., "start recording", "change generator") on this queue for the Sequencer to process in a musically quantized way.
-   **`midi_queue`**: The Sequencer places MIDI messages on this queue. The MidiPlayer consumes them.
-   **`gui_queue`**: The backend (Sequencer, Looper) places simple notifications on this queue (e.g., "blink light", "recording started") for the GUI to consume.

## 2. Project Structure

The project follows a standard `src` layout:

```
creative-midi-looper/
├── src/                    # All source code
│   ├── engine/             # Core sequencer and timing logic
│   ├── generators/         # All musical pattern generation algorithms
│   ├── gui/                # DearPyGui main window and UI logic
│   ├── looper/             # The MIDI looper module
│   ├── midi/               # MIDI port handling
│   └── theory/             # Music theory helpers (scales, durations)
├── docs/                   # Documentation
├── presets/                # User-saved presets
├── tests/                  # Pytest test suite
├── main.py                 # Main application entry point
└── requirements.txt        # Dependencies
```

## 3. Component Details

This section provides a more detailed look at the key classes in the project.

### Pattern Generators (`src/generators/`)
- **`BaseGenerator`**: An abstract base class for all generators. Defines the interface that all generators must implement.
- **`RandomGenerator` / `RandomGeneratorV2`**: Generate notes randomly, constrained by musical scale and other parameters.
- **`EuclideanGenerator`**: Generates rhythmic patterns using the Euclidean algorithm.
- **`DualEuclideanGenerator`**: Runs two independent Euclidean generators to create complex polyrhythms.

### Sequencer Engine (`src/engine/`)
- **`Sequencer`**: The high-precision heart of the application. Runs in its own thread, manages the master musical clock, calls the active generator to get notes, and schedules all MIDI events.
- **`MIDIHandler` (now `MidiPlayer` in `src/midi/`)**: Handles all communication with MIDI ports. Runs in a dedicated thread to send messages from the `midi_queue` without blocking the rest of the application.
- **`PatternManager` (now part of `MainWindow`)**: The logic for saving and loading presets is managed by the `MainWindow` class.

### Music Theory (`src/theory/`)
- **`scales.py`**: Contains definitions for over 14 musical scales and modes, from standard Western modes to exotic scales like Hirajoshi and Hungarian Minor.
- **`duration.py`**: Manages the duration of generated notes.

### User Interface (`src/gui/`)
- **`MainWindow`**: Manages the entire GUI using DearPyGui. It is responsible for building all widgets, handling user input, and dispatching commands to the backend.

### Configuration and Presets
- **`config.json`**: Main configuration file.
- **`presets/`**: Directory for user-saved presets.

## 4. Extension Points

The application is designed to be extensible in several ways:
- **New Pattern Generators**: Create a new class that inherits from `BaseGenerator` and implement the `generate()` method.
- **New Scales**: Add new scale definitions to the `SCALES` dictionary in `src/theory/scales.py`.
- **Custom Note Filters**: (Future) Add new modules to filter or modify notes after they are generated.
- **MIDI Effects**: (Future) Add new MIDI effect modules to process messages before they are sent.

## 5. Testing Strategy

A robust test suite is crucial for maintaining the stability of this real-time application. The project uses `pytest` as its test runner and follows a multi-layered testing strategy.

### Running Tests

To run the full test suite, execute the following command from the project root:

```bash
pytest
```

### Coverage Analysis

To run the tests and generate a coverage report, use the `pytest-cov` plugin:

```bash
pytest --cov=src
```

### Types of Tests

-   **Unit Tests:** These are located in files like `test_random_generator.py` or `test_scales.py`. They test a single class or function in isolation to verify its internal logic.
-   **Integration Tests:** Found in `test_integration.py` and `test_synchronization.py`, these tests verify the interaction between multiple components (e.g., ensuring the Sequencer and Looper stay in sync).
-   **Headless UI Tests:** The project uses a special technique to test the GUI's application logic without actually rendering a window. In `test_main_window.py` and `test_main_window_callbacks.py`, the `dearpygui` library is heavily mocked. This allows us to call the UI's callback functions (e.g., `on_bpm_changed`) and assert that the backend state is updated correctly. This is the preferred way to test UI features.
