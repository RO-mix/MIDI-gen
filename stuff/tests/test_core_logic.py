import pytest
import queue
import mido
import os
import json
import tempfile
import time
from unittest.mock import patch, MagicMock

# We need to patch the start method of the threads to prevent them from running
@patch('src.engine.sequencer.Sequencer.start', MagicMock())
@patch('src.midi.midi_handler.MidiPlayer.start', MagicMock())
@patch('atexit.register', MagicMock())
def get_clean_main_window():
    """Factory function to get a MainWindow instance with threads patched."""
    from src.gui.main_window import MainWindow
    return MainWindow()

@patch('time.perf_counter')
def test_sequencer_tick_uses_generator_to_produce_midi(mock_perf_counter):
    """
    Tests if the sequencer correctly generates notes from a generator using a mocked timeline.
    """
    app = get_clean_main_window()
    sequencer = app.sequencer
    midi_queue = app.midi_queue

    # Mock time to control the sequencer's clock
    # play() consumes the first value, then each _tick consumes one.
    mock_perf_counter.side_effect = [100.0, 100.0, 100.5, 101.0, 101.5]

    # Setup a simple Euclidean generator
    from src.generators.euclidean_generator import EuclideanGenerator
    generator = EuclideanGenerator(steps=4, pulses=2, note=60, velocity=100) # Pattern: [1, 0, 1, 0]
    sequencer.set_generator(generator)
    sequencer.set_bpm(120) # 2 beats per second
    sequencer.play() # Sets up last_tick_time, consumes 100.0

    # --- Run the test for one full pattern cycle---
    # Manually advance the beat and call tick, simulating the run loop.
    # The generator has 4 steps, so we tick 4 times.
    for _ in range(4):
        sequencer._tick()
        sequencer.current_beat += generator.rate # Advance time by one step

    # --- Check the generated MIDI messages ---
    note_on_messages = [msg for msg in list(midi_queue.queue) if msg.type == 'note_on']

    assert len(note_on_messages) == 2
    assert note_on_messages[0].note == 60
    assert note_on_messages[1].note == 60


@patch('src.gui.main_window.dpg')
def test_save_preset_to_file_and_load_in_new_instance(mock_dpg):
    """
    Tests the preset saving and loading logic without a running UI.
    """
    # Configure the mock before it's used
    mock_dpg.does_item_exist.return_value = True

    # 1. Setup a MainWindow instance with patched threads
    app = get_clean_main_window()

    # 2. Change some state
    app.sequencer.set_bpm(150)
    app.on_generator_selected(None, "Евклидов") # Switch to Euclidean
    app.generator.update_params(steps=8, pulses=5, note=72)

    # 3. Save the state to a temporary file
    state = app._gather_current_state()
    with tempfile.NamedTemporaryFile(mode='w+', delete=False, suffix=".json") as tmp:
        json.dump(state, tmp)
        tmp_path = tmp.name

    # 4. Create a new MainWindow instance and load the state
    new_app = get_clean_main_window()
    new_app.load_preset(tmp_path)

    # 5. Assert that the backend state was loaded correctly
    assert new_app.sequencer.bpm == 150
    from src.generators.euclidean_generator import EuclideanGenerator
    assert isinstance(new_app.generator, EuclideanGenerator)
    assert new_app.generator.steps == 8
    assert new_app.generator.pulses == 5

    # 6. Verify that the UI update methods were called with correct values
    mock_dpg.set_value.assert_any_call("ui_bpm", 150)
    mock_dpg.set_value.assert_any_call("ui_euclidean_steps", 8)

    # 7. Cleanup
    os.remove(tmp_path)
