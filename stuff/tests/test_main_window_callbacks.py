import pytest
from unittest.mock import patch, MagicMock

# Note: The 'mock_dpg' and 'main_window' fixtures are now defined in tests/conftest.py
# and are available automatically to all tests in this file.

def test_bpm_slider_updates_sequencer_state(main_window):
    """
    Tests that moving the BPM slider calls the callback, which updates the sequencer's BPM.
    """
    # Check initial state
    assert main_window.sequencer.bpm == 120

    # Simulate the dpg.add_slider callback firing
    # In the real app, dpg passes the new value as the second argument.
    main_window.on_bpm_changed(sender=None, bpm=145)

    # Check that the sequencer's state has been updated
    assert main_window.sequencer.bpm == 145

def test_channel_slider_updates_sequencer_state(main_window):
    """
    Tests that moving the MIDI channel slider updates the sequencer's channel.
    Note: The callback is a lambda, so we test the underlying method call.
    """
    assert main_window.sequencer.midi_channel == 0

    # The UI shows 1-16, but the backend is 0-15.
    main_window.sequencer.set_midi_channel(10 - 1)

    assert main_window.sequencer.midi_channel == 9

def test_musical_context_updates_generator_scale(main_window):
    """
    Tests that changing the root note or scale name updates the notes available to the generator.
    """
    # Initial state: C Major. Note 61 (C#) should not be in the scale.
    initial_notes = main_window.generator.scale_notes
    assert 60 in initial_notes # C
    assert 61 not in initial_notes # C#

    # Change root note to D (2) by calling the new testable method
    main_window._update_musical_context("ui_root_note", "D")

    # New state: D Major. Note 61 (C#) should now BE in the scale.
    new_notes = main_window.generator.scale_notes
    assert 60 not in new_notes # C
    assert 61 in new_notes # C#

def test_generator_selection_changes_generator_instance(main_window):
    """
    Tests that selecting a new generator from the dropdown changes the active generator instance.
    """
    from src.generators.random_generator import RandomGenerator
    from src.generators.euclidean_generator import EuclideanGenerator

    # Initial state
    assert isinstance(main_window.generator, RandomGenerator)

    # Change generator to Euclidean
    main_window.on_generator_selected(None, "Евклидов")

    # Check that the generator instance has changed
    assert isinstance(main_window.generator, EuclideanGenerator)
    assert main_window.sequencer.generator == main_window.generator

def test_looper_capture_sends_correct_command(main_window):
    """
    Tests that the looper capture button sends the correct command to the sequencer.
    """
    # Mock dpg.get_value to return predictable UI settings
    with patch('src.gui.main_window.dpg.get_value', side_effect=['След. бит', '2 такта', False, True]):
        main_window._looper_capture()

    assert not main_window.command_queue.empty()
    command, args = main_window.command_queue.get_nowait()

    assert command == 'looper_capture'
    assert args['quantize'] == 'beat'
    assert args['duration_beats'] == 8.0
    assert args['is_through'] is False
    assert args['is_overdub'] is True

def test_looper_toggle_play_sends_correct_command(main_window, mocker):
    """
    Tests that the looper play/stop button sends the correct command.
    """
    mocker.patch('src.gui.main_window.dpg.set_item_label') # Prevent segfault
    with patch('src.gui.main_window.dpg.get_value', side_effect=['Моментально', True]):
        main_window._looper_toggle_play()

    assert not main_window.command_queue.empty()
    command, args = main_window.command_queue.get_nowait()

    assert command == 'looper_toggle_play'
    assert args['quantize'] == 'instant'
    assert args['is_through'] is True

def test_looper_double_sends_correct_command(main_window):
    """
    Tests that the looper double button sends the correct command.
    """
    with patch('src.gui.main_window.dpg.get_value', return_value='След. такт'):
        main_window._looper_double()

    assert not main_window.command_queue.empty()
    command, args = main_window.command_queue.get_nowait()

    assert command == 'looper_double'
    assert args['quantize'] == 'bar'

def test_looper_toggle_record_sends_correct_command(main_window):
    """
    Tests that the looper record button sends the correct command.
    """
    # Mock dpg.get_value to return predictable UI settings
    # quantize, overdub, record_length, through
    with patch('src.gui.main_window.dpg.get_value', side_effect=['След. бит', True, '8 тактов', False]):
        main_window._looper_toggle_record()

    assert not main_window.command_queue.empty()
    command, args = main_window.command_queue.get_nowait()

    assert command == 'looper_toggle_record'
    assert args['quantize'] == 'beat'
    assert args['is_overdub'] is True
    assert args['length_beats'] == 32.0
    assert args['is_through'] is False

def test_update_and_refresh_scale_updates_generator(main_window):
    """
    Tests that the _update_and_refresh_scale callback correctly updates the generator params.
    """
    # Spy on the generator's update_params method
    with patch.object(main_window.generator, 'update_params') as mock_update_params:
        with patch.object(main_window, '_update_generator_scale_notes') as mock_update_scale:
            main_window._update_and_refresh_scale('min_note', 48)

            mock_update_params.assert_called_once_with(min_note=48)
            mock_update_scale.assert_called_once()
