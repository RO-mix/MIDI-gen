from unittest.mock import Mock, patch
import pretty_midi
import pytest
import tempfile
import copy
import os

from src.looper.looper_module import LooperModule
from src.theory.scales import SCALES

# Note: The `looper_test_data` fixture is now defined in tests/conftest.py

# === Test Capture and Basic State ===

def test_capture_seed_creates_beat_native_loop(looper_test_data):
    """Test that capturing a seed correctly creates a beat-native loop."""
    looper = looper_test_data['looper']
    assert looper.loop is None
    looper_test_data['capture_default_seed']()

    assert looper.loop is not None
    assert isinstance(looper.loop, list)
    assert len(looper.loop) == 3
    assert looper.bpm == looper_test_data['bpm']
    assert looper.pristine_loop is not None
    assert looper.loop_duration_beats == looper_test_data['capture_duration_beats']

    note = looper.loop[0]
    assert note['start_beat'] == 0.0
    assert note['end_beat'] == 0.5

def test_capture_truncates_notes_to_duration(looper_test_data):
    """Test that notes extending beyond the capture duration are truncated in beats."""
    looper = looper_test_data['looper']
    notes_with_long_note = [
        {'pitch': 60, 'velocity': 100, 'start': 0.0, 'end': 1.0},
        {'pitch': 62, 'velocity': 100, 'start': 1.5, 'end': 2.5},
    ]
    capture_duration = 2.0
    looper.capture_seed(notes_with_long_note, [], 60, [], looper_test_data['bpm'], capture_duration)

    last_note = looper.loop[-1]
    assert last_note['end_beat'] == capture_duration
    assert looper.loop_duration_beats == capture_duration

def test_capture_empty_seed_resets_state(looper_test_data):
    """Test that capturing an empty seed clears the loop and duration."""
    looper = looper_test_data['looper']
    looper_test_data['capture_default_seed']()
    assert looper.loop is not None

    looper.capture_seed([], [], 0, [], 120, 4.0)
    assert looper.loop is None
    assert looper.loop_duration_beats == 0.0

# === Test Loop Manipulation ===

def test_double_loop(looper_test_data):
    """Test that the loop duration, note count, and pristine version are doubled."""
    looper = looper_test_data['looper']
    looper_test_data['capture_default_seed']()
    original_duration_beats = looper.loop_duration_beats
    original_note_count = len(looper.loop)

    looper.double_loop()

    assert looper.loop_duration_beats == original_duration_beats * 2
    assert len(looper.loop) == original_note_count * 2
    assert len(looper.pristine_loop) == original_note_count * 2

    shifted_note = looper.loop[-1]
    assert shifted_note['start_beat'] == 1.0 + original_duration_beats

def test_split_loop_keep_first_half(looper_test_data):
    """Test splitting the loop and keeping the first half."""
    looper = looper_test_data['looper']
    looper_test_data['capture_default_seed']()
    looper.split_loop(keep_first_half=True)

    assert len(looper.loop) == 2
    assert looper.loop_duration_beats == looper_test_data['capture_duration_beats'] / 2.0
    assert looper.loop[1]['end_beat'] == 1.0

def test_split_loop_keep_second_half(looper_test_data):
    """Test splitting and keeping the second half, ensuring notes are time-shifted."""
    looper = looper_test_data['looper']
    notes = [
        {'pitch': 60, 'velocity': 100, 'start': 0.0, 'end': 0.5},
        {'pitch': 64, 'velocity': 100, 'start': 0.5, 'end': 1.5},
        {'pitch': 67, 'velocity': 100, 'start': 1.5, 'end': 1.9},
    ]
    looper.capture_seed(notes, [], 60, [], looper_test_data['bpm'], looper_test_data['capture_duration_beats'])
    looper.split_loop(keep_first_half=False)

    assert len(looper.loop) == 2
    assert looper.loop_duration_beats == looper_test_data['capture_duration_beats'] / 2.0
    assert looper.loop[0]['pitch'] == 64
    assert looper.loop[0]['start_beat'] == pytest.approx(0.0)
    assert looper.loop[0]['end_beat'] == pytest.approx(0.5)
    assert looper.loop[1]['pitch'] == 67
    assert looper.loop[1]['start_beat'] == pytest.approx(0.5)

# === Test Quantization ===

def test_quantize_notes_preserves_duration(looper_test_data):
    """Test that quantization snaps the start time but preserves the original duration in beats."""
    looper = looper_test_data['looper']
    notes = [
        {'pitch': 50, 'velocity': 100, 'start': 0.0, 'end': 0.1},
        {'pitch': 60, 'velocity': 100, 'start': 0.28, 'end': 0.78}
    ]
    looper.capture_seed(notes, [], 60, [], looper_test_data['bpm'], 1.0)
    original_duration_beats = looper.pristine_loop[1]['end_beat'] - looper.pristine_loop[1]['start_beat']
    looper.quantize_notes(grid_resolution_beats=0.25)

    assert looper.is_quantized
    quantized_note = looper.loop[1]
    assert quantized_note['start_beat'] == pytest.approx(0.25)
    quantized_duration = quantized_note['end_beat'] - quantized_note['start_beat']
    assert quantized_duration == pytest.approx(original_duration_beats)

def test_unquantize_reverts_loop(looper_test_data):
    """Test that unquantize reverts the active loop to the pristine version."""
    looper = looper_test_data['looper']
    notes = [
        {'pitch': 50, 'velocity': 100, 'start': 0.0, 'end': 0.1},
        {'pitch': 62, 'velocity': 100, 'start': 0.28, 'end': 0.78}
    ]
    looper.capture_seed(notes, [], 60, [], looper_test_data['bpm'], 1.0)
    pristine_start_beat = looper.pristine_loop[1]['start_beat']
    looper.quantize_notes(grid_resolution_beats=0.25)
    assert looper.loop[1]['start_beat'] != pristine_start_beat

    looper.unquantize_notes()
    assert not looper.is_quantized
    assert looper.loop[1]['start_beat'] == pristine_start_beat

# === Test Recording & Overdubbing ===

def test_new_recording_replaces_loop_on_stop(looper_test_data):
    """Test that a new recording replaces the loop upon stopping."""
    looper = looper_test_data['looper']
    looper_test_data['capture_default_seed']()
    assert looper.loop is not None
    original_loop_notes = copy.deepcopy(looper.loop)
    looper.start_recording(record_length_beats=2.0, start_beat=0.0, overdub=False)
    assert looper.is_recording
    assert looper.loop is not None
    assert looper.loop == original_loop_notes
    looper.add_live_note({'pitch': 99, 'start_beat': 0.5, 'duration_beats': 0.5, 'velocity': 100})
    looper.stop_recording()

    assert looper.loop is not None
    assert len(looper.loop) == 1
    assert looper.loop[0]['pitch'] == 99

def test_stop_recording_creates_beat_native_loop(looper_test_data):
    """Test that stopping a recording session finalizes a beat-native loop."""
    looper = looper_test_data['looper']
    looper.start_recording(record_length_beats=4.0, start_beat=0.0, overdub=False)
    looper.add_live_note({'pitch': 60, 'start_beat': 0.5, 'duration_beats': 0.5, 'velocity': 100})
    looper.add_live_note({'pitch': 62, 'start_beat': 1.5, 'duration_beats': 0.5, 'velocity': 100})
    looper.stop_recording()

    assert not looper.is_recording
    assert looper.loop is not None
    assert len(looper.loop) == 2
    assert looper.loop_duration_beats == 4.0
    assert looper.loop[0]['start_beat'] == 0.5
    assert looper.pristine_loop[0]['end_beat'] == 1.0

def test_overdub_adds_notes_to_existing_loop(looper_test_data):
    """Test that overdubbing merges new notes into the current loop."""
    looper = looper_test_data['looper']
    looper_test_data['capture_default_seed']()
    original_note_count = len(looper.loop)
    original_duration_beats = looper.loop_duration_beats
    looper.start_recording(record_length_beats=99.0, start_beat=0.0, overdub=True)
    assert looper.is_overdubbing
    assert looper.record_length_beats == original_duration_beats
    looper.add_live_note({'pitch': 72, 'start_beat': 0.25, 'duration_beats': 0.5, 'velocity': 120})
    looper.stop_recording()

    assert not looper.is_overdubbing
    assert len(looper.loop) == original_note_count + 1
    assert len(looper.pristine_loop) == original_note_count + 1

# === Test Tick and Playback ===

def test_tick_plays_notes_at_correct_beat(looper_test_data):
    """Test that the tick method triggers notes based on beat-native timing."""
    looper = looper_test_data['looper']
    mock_sequencer = looper_test_data['mock_sequencer']
    looper_test_data['capture_default_seed']()
    looper.play(start_beat=0.0)

    looper.tick(master_beat=0.0)
    mock_sequencer.play_looper_note.assert_not_called()

    looper.tick(master_beat=0.01)
    mock_sequencer.play_looper_note.assert_called_once()
    args, _ = mock_sequencer.play_looper_note.call_args
    assert args[0].note == 60

    looper.tick(master_beat=0.4)
    mock_sequencer.play_looper_note.assert_called_once()

    looper.tick(master_beat=0.51)
    assert mock_sequencer.play_looper_note.call_count == 2
    args, _ = mock_sequencer.play_looper_note.call_args
    assert args[0].note == 64

def test_tick_schedules_correct_note_off(looper_test_data):
    """Test that note-off events are scheduled with the correct duration in beats."""
    looper = looper_test_data['looper']
    mock_sequencer = looper_test_data['mock_sequencer']
    looper_test_data['capture_default_seed']()
    looper.play(start_beat=10.0)

    looper.tick(master_beat=10.0)
    mock_sequencer.play_looper_note.assert_not_called()

    master_beat_at_trigger = 10.01
    looper.tick(master_beat=master_beat_at_trigger)
    mock_sequencer.play_looper_note.assert_called_once()

    note_duration_beats = looper.loop[0]['end_beat'] - looper.loop[0]['start_beat']
    expected_off_beat = master_beat_at_trigger + note_duration_beats
    mock_sequencer.schedule_external_note_off.assert_called_once()
    args, _ = mock_sequencer.schedule_external_note_off.call_args
    assert args[0] == 60
    assert args[1] == pytest.approx(expected_off_beat)

# === Test File I/O ===

def test_save_and_load_loop(looper_test_data):
    """Test saving and loading a loop."""
    looper = looper_test_data['looper']
    looper_test_data['capture_default_seed']()
    looper.quantize_notes(0.25)

    with tempfile.NamedTemporaryFile(suffix='.mid', delete=True) as tmp:
        filepath = tmp.name
        looper.save(filepath)
        assert os.path.exists(filepath)

        saved_pm = pretty_midi.PrettyMIDI(filepath)
        saved_note = saved_pm.instruments[0].notes[0]
        assert saved_note.start == pytest.approx(0.0 * looper_test_data['beats_to_seconds'])

        new_looper = LooperModule()
        notes_from_file = [{'pitch': n.pitch, 'velocity': n.velocity, 'start': n.start, 'end': n.end} for n in saved_pm.instruments[0].notes]
        new_looper.load_loop(notes_from_file, looper_test_data['bpm'])

        assert new_looper.loop is not None
        assert new_looper.bpm == looper.bpm
        assert len(new_looper.loop) == len(looper.loop)
        loaded_note = new_looper.loop[0]
        assert loaded_note['start_beat'] == pytest.approx(looper.loop[0]['start_beat'])

# === Test Edge Cases and Other Logic ===

def test_manipulation_on_empty_loop_is_safe(looper_test_data):
    """Tests that calling manipulation functions on an empty loop does not crash."""
    looper = looper_test_data['looper']
    assert looper.loop is None

    # These calls should do nothing and not raise errors
    looper.double_loop()
    looper.split_loop(keep_first_half=True)
    looper.quantize_notes(0.25)
    looper.unquantize_notes()
    looper.generate_variations()

    assert looper.loop is None

def test_pad_mode_disables_note_off_scheduling(looper_test_data):
    """Tests that when pad_mode is on, note_off events are not scheduled."""
    looper = looper_test_data['looper']
    mock_sequencer = looper_test_data['mock_sequencer']
    looper_test_data['capture_default_seed']()

    looper.set_pad_mode(True)
    assert looper.pad_mode is True

    looper.play(start_beat=10.0)
    looper.tick(master_beat=10.0)
    looper.tick(master_beat=10.01) # Trigger note

    # Note should be played, but note_off should NOT be scheduled
    mock_sequencer.play_looper_note.assert_called_once()
    mock_sequencer.schedule_external_note_off.assert_not_called()
