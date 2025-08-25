import pytest
from unittest.mock import patch

# This file contains shared fixtures for pytest.
# Fixtures defined here will be available to all tests in the 'tests' directory
# without needing to be imported.

@pytest.fixture(autouse=True)
def mock_dpg(mocker):
    """
    Mocks all necessary dearpygui functions before they are imported by main_window.
    This allows us to instantiate MainWindow without a running GUI.
    The autouse=True flag ensures this fixture is used for every test, automatically.
    """
    mocker.patch('dearpygui.dearpygui.create_context')
    mocker.patch('dearpygui.dearpygui.setup_dearpygui')
    mocker.patch('dearpygui.dearpygui.show_viewport')
    mocker.patch('dearpygui.dearpygui.set_value')
    mocker.patch('dearpygui.dearpygui.get_item_label')
    mocker.patch('dearpygui.dearpygui.does_item_exist', return_value=True)
    mocker.patch('dearpygui.dearpygui.configure_item')
    mocker.patch('dearpygui.dearpygui.is_dearpygui_running', return_value=True)
    mocker.patch('dearpygui.dearpygui.destroy_context')

@pytest.fixture
def main_window(mocker):
    """
    Provides a fully mocked MainWindow instance for testing UI logic.
    It patches the backend threads so they don't start.
    """
    # We need to import MainWindow here, after dpg has been mocked.
    from src.gui.main_window import MainWindow

    # We also need to mock the threads so they don't start
    mocker.patch('src.midi.midi_handler.MidiPlayer.start')
    mocker.patch('src.engine.sequencer.Sequencer.start')
    mocker.patch('atexit.register')
    return MainWindow()

@pytest.fixture
def looper_test_data(mocker):
    """A fixture to set up a LooperModule instance and common test data."""
    from src.looper.looper_module import LooperModule
    from src.theory.scales import SCALES
    from unittest.mock import Mock
    import copy

    mocker.patch('src.looper.looper_module.logging.getLogger')

    looper = LooperModule()
    looper.gui_queue = Mock()
    mock_sequencer = Mock()
    mock_sequencer.current_beat = 0.0
    looper.set_sequencer(mock_sequencer)

    bpm = 120.0
    beats_to_seconds = 60.0 / bpm
    c_major_scale_notes = [n for n in range(128) if n % 12 in [0, 2, 4, 5, 7, 9, 11]]
    c_major_degrees = SCALES['Major']
    root_note = 60
    sample_notes_beats = [
        {'pitch': 60, 'velocity': 100, 'start': 0.0, 'end': 0.5},
        {'pitch': 64, 'velocity': 100, 'start': 0.5, 'end': 1.0},
        {'pitch': 67, 'velocity': 100, 'start': 1.0, 'end': 1.5},
    ]
    capture_duration_beats = 2.0

    def _capture_default_seed():
        looper.capture_seed(
            sample_notes_beats,
            c_major_scale_notes,
            root_note,
            c_major_degrees,
            bpm,
            capture_duration_beats
        )

    return {
        "looper": looper,
        "mock_sequencer": mock_sequencer,
        "bpm": bpm,
        "beats_to_seconds": beats_to_seconds,
        "capture_duration_beats": capture_duration_beats,
        "capture_default_seed": _capture_default_seed,
    }
