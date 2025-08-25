import pytest
import json
from unittest.mock import mock_open, patch

# Note: The 'mock_dpg' and 'main_window' fixtures are now defined in tests/conftest.py
# and are available automatically to all tests in this file.

from src.gui.main_window import MainWindow

def test_gather_state_for_preset(main_window):
    # Change state to something non-default
    main_window.sequencer.set_bpm(155)
    main_window.root_note = 5 # F
    main_window.scale_name = "Minor Pentatonic"
    main_window.on_generator_selected(None, "Евклидов")
    main_window.generator.update_params(steps=11, pulses=7, duration_bias=0.8)

    state = main_window._gather_current_state()

    assert state["app_state"]["bpm"] == 155
    assert state["app_state"]["root_note"] == 5
    assert state["app_state"]["scale_name"] == "Minor Pentatonic"
    assert state["selected_generator"] == "Евклидов"
    assert state["generator_settings"]["steps"] == 11
    assert state["generator_settings"]["pulses"] == 7
    assert state["generator_settings"]["duration_bias"] == 0.8

def test_load_preset_applies_state(main_window, mocker):
    preset_data = {
        "app_state": {
            "bpm": 99,
            "root_note": 2,
            "scale_name": "Dorian"
        },
        "selected_generator": "Двойной Евклидов",
        "generator_settings": {
            "steps_a": 5,
            "pulses_a": 2,
            "duration_bias_a": 0.2,
            "duration_bias_b": 0.9
        }
    }
    preset_json = json.dumps(preset_data)

    # Mock file reading
    mocker.patch("builtins.open", mock_open(read_data=preset_json))

    # Spy on the key methods.
    update_ui_spy = mocker.spy(main_window, "_update_all_ui_from_state")

    main_window.load_preset("dummy/path.json")

    # Check that the generator was switched
    assert main_window.current_generator_name == "Двойной Евклидов"

    # Check that the final state of the generator is correct
    from src.generators.dual_euclidean_generator import DualEuclideanGenerator
    assert isinstance(main_window.generator, DualEuclideanGenerator)
    settings = preset_data["generator_settings"]
    assert main_window.generator.steps_a == settings["steps_a"]
    assert main_window.generator.pulses_a == settings["pulses_a"]
    assert main_window.generator.duration_bias_a == settings["duration_bias_a"]
    assert main_window.generator.duration_bias_b == settings["duration_bias_b"]


    # Check that the main app state was updated
    assert main_window.sequencer.bpm == 99
    assert main_window.root_note == 2
    assert main_window.scale_name == "Dorian"

    # Check that the UI was told to update
    update_ui_spy.assert_called_once()
