# Creative MIDI Generator

This project is a MIDI pattern generator designed for creative music production. It uses various algorithmic techniques to generate musical ideas in real-time.

## Contributing

We welcome contributions! If you would like to contribute to the project, please read our [Developer Documentation](docs/DEVELOPMENT.md) for information on the project's architecture, component details, and how to extend the application with new features.

## Requirements

- Python 3.x
- A virtual MIDI port or a physical MIDI device to receive MIDI messages.

## Installation

1. Clone the repository.
2. It is recommended to create a virtual environment.
3. Install the required dependencies:
   ```bash
   pip install -r requirements.txt
   ```

## Usage

1. Run the application:
   ```bash
   python main.py
   ```
2. **Select a MIDI Port**: In the "MIDI" section, choose an available MIDI output port from the dropdown menu. The application will send MIDI data to this port.
3. **Start the Sequencer**: In the "Секвенсор" (Sequencer) section, press the "Старт" (Start) button.
4. **Choose a Generator**: In the "Генератор" (Generator) section, select a generator type and adjust its parameters to shape the output.
5. **Set the Musical Context**: Use the "Музыкальный контекст" (Musical Context) section to choose a root note and scale to constrain the generated notes.

## Implemented Generators

The generator is designed to be extensible. The following generators are currently implemented:

- **Случайный (Random)**: Generates random notes within a specified range and scale.
- **Евклидов (Euclidean)**: Generates rhythmic patterns based on the Euclidean algorithm.
- **Двойной Евклидов (Dual Euclidean)**: Runs two independent Euclidean generators simultaneously to create complex polyrhythms.

## Features

- **Preset Management**: Save and load your favorite generator configurations. Click the `+` button to open the "Save New Preset" dialog, give your preset a name, and click "Save". Your saved presets will appear in the dropdown menu for easy recall.
- **MIDI Channel Control**: Set the outgoing MIDI channel (1-16) for the sequencer.
- **Modular Generators**: The architecture makes it easy to add new generator types.
- **Looper & Live Timeline**:
    - **Live View**: A dynamic timeline visualizes the generator's output in real-time over a 4-bar window, with automatic vertical zooming to fit the note range.
    - **Stream Record**: Record the generator's live output into a new loop. You can set a recording length from 1 to 64 bars, after which it will automatically stop and begin playback.
    - **Capture**: Instantly capture a snapshot of the generator's current pattern.
    - **Playback & Manipulation**: Play captured or recorded loops, which can be manipulated (doubled, split, quantized).
    - **Enhanced Visualization**: Notes in the timeline are rendered in orange, with their brightness corresponding to velocity, and are drawn with a thicker, slightly 3D style for better visibility.