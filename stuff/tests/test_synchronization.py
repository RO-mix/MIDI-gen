import time
import queue
import mido
import logging
import sys
import pytest
from unittest.mock import patch

from src.engine.sequencer import Sequencer
from src.looper.looper_module import LooperModule
from src.generators.base import BaseGenerator

# Configure logging for tests
logging.basicConfig(level=logging.DEBUG, stream=sys.stdout, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')

# Helper class for testing
class SimpleTestGenerator(BaseGenerator):
    def __init__(self, note, rate=4.0, duration=1.0):
        self.note = note
        self.rate = rate
        self.duration = duration

    def generate(self, current_beat):
        msg = mido.Message('note_on', note=self.note, velocity=100)
        return [(msg, self.duration)], self.rate

    def set_scale_notes(self, notes): pass # Dummy method to satisfy interface

def test_passthrough_note_is_played_and_recorded():
    """
    Tests that when the sequencer generates a note while the looper is recording,
    the note is (a) sent to the MIDI queue, (b) recorded by the looper, and
    (c) its note-off is scheduled by the sequencer.
    """
    midi_queue = queue.Queue()
    looper = LooperModule()
    sequencer = Sequencer(midi_queue, looper=looper)
    looper.set_sequencer(sequencer)

    class MockGenerator(BaseGenerator):
        def generate(self, current_beat):
            msg = mido.Message('note_on', note=60, velocity=100)
            return ([(msg, 1.0)], 4.0)
    sequencer.set_generator(MockGenerator())

    looper.start_recording(record_length_beats=4.0, start_beat=0.0, overdub=False)
    sequencer.play()
    sequencer.next_note_beat = 0.0

    sequencer._run_tick()

    assert not midi_queue.empty(), "MIDI queue should have a note_on message"
    note_on_msg = midi_queue.get_nowait()
    assert note_on_msg.type == 'note_on'
    assert note_on_msg.note == 60

    assert len(looper.recorded_notes) == 1
    recorded_note = looper.recorded_notes[0]
    assert recorded_note['pitch'] == 60
    assert recorded_note['duration_beats'] == pytest.approx(1.0)
    assert recorded_note['start_beat'] == pytest.approx(sequencer.current_beat, abs=0.01)

    assert len(sequencer.scheduled_events) == 1
    off_time, _, note_off_msg, _ = sequencer.scheduled_events[0]
    assert note_off_msg.type == 'note_off'
    assert off_time == pytest.approx(sequencer.current_beat + 1.0, abs=0.01)

@patch('time.sleep')
def test_sequencer_and_looper_clock_synchronization(mock_sleep):
    """
    Tests that the looper and sequencer clocks are perfectly synchronized
    by having them play a note on the exact same beat.
    """
    midi_queue = queue.Queue()
    gui_queue = queue.Queue()
    looper = LooperModule(gui_queue=gui_queue)
    sequencer = Sequencer(midi_queue, gui_queue=gui_queue, looper=looper, bpm=60)
    looper.set_sequencer(sequencer)
    looper.set_bpm(60)

    generator = SimpleTestGenerator(note=60, rate=4.0)
    sequencer.set_generator(generator)
    sequencer.next_note_beat = 2.0

    loop_notes = [
        {'pitch': 62, 'velocity': 100, 'start': 2.0, 'end': 3.0},
        {'pitch': 1, 'velocity': 0, 'start': 3.9, 'end': 4.0},
    ]
    looper.load_loop(notes=loop_notes, bpm=60)

    sequencer.play()
    looper.play()

    for _ in range(1001):
        sequencer._run_tick()

    messages = []
    while not midi_queue.empty():
        messages.append(midi_queue.get_nowait())

    note_on_messages = [m for m in messages if m.type == 'note_on']
    assert len(note_on_messages) == 2, "Should be two note_on messages"

    notes = sorted([m.note for m in note_on_messages])
    assert notes == [60, 62], "Should have one note from generator (60) and one from looper (62)"

@patch('time.sleep')
def test_hanging_notes_on_recapture(mock_sleep):
    """
    Tests that when a new loop is captured, the note_off events from the
    previous loop are cancelled, preventing hanging notes.
    """
    midi_queue = queue.Queue()
    gui_queue = queue.Queue()
    looper = LooperModule(gui_queue=gui_queue)
    sequencer = Sequencer(midi_queue, gui_queue=gui_queue, looper=looper, bpm=60)
    looper.set_sequencer(sequencer)
    looper.set_bpm(60)
    sequencer.play()

    loop1_notes = [{'pitch': 60, 'velocity': 100, 'start': 0.0, 'end': 2.0}]
    looper.load_loop(notes=loop1_notes, bpm=60)
    looper.play()

    for _ in range(500):
        sequencer._run_tick()

    note_off_events = [e for e in sequencer.scheduled_events if e[2].type == 'note_off' and e[2].note == 60]
    assert len(note_off_events) == 1
    assert note_off_events[0][0] == pytest.approx(2.0, abs=1e-2)

    looper.capture_seed(notes=[], scale_notes=[], root_note=60, scale_degrees=[], bpm=60, duration_beats=4.0)

    note_off_events = [e for e in sequencer.scheduled_events if e[2].type == 'note_off' and e[2].note == 60]
    assert len(note_off_events) == 0, "The note_off for the old loop should have been cancelled."

@patch('time.sleep')
def test_hanging_notes_on_double_loop(mock_sleep):
    """
    Tests that when a loop is transformed, note_off events are cancelled.
    """
    midi_queue = queue.Queue()
    gui_queue = queue.Queue()
    looper = LooperModule(gui_queue=gui_queue)
    sequencer = Sequencer(midi_queue, gui_queue=gui_queue, looper=looper, bpm=60)
    looper.set_sequencer(sequencer)
    looper.set_bpm(60)
    sequencer.play()

    loop_notes = [{'pitch': 60, 'velocity': 100, 'start': 0.0, 'end': 2.0}]
    looper.load_loop(notes=loop_notes, bpm=60)
    looper.play()
    for _ in range(500):
        sequencer._run_tick()
    assert len(sequencer.scheduled_events) == 1

    looper.double_loop()
    assert len(sequencer.scheduled_events) == 0

@patch('time.sleep')
def test_recording_duration_with_custom_bpm(mock_sleep):
    """
    Tests that the looper stops recording at the correct beat with a non-default BPM.
    """
    bpm = 240
    midi_queue = queue.Queue()
    gui_queue = queue.Queue()
    looper = LooperModule(gui_queue=gui_queue)
    looper.set_bpm(bpm)
    sequencer = Sequencer(midi_queue, gui_queue=gui_queue, looper=looper, bpm=bpm)
    looper.set_sequencer(sequencer)
    sequencer.play()

    looper.start_recording(record_length_beats=4.0, start_beat=sequencer.current_beat)
    assert looper.is_recording

    for _ in range(499):
        sequencer._run_tick()
    assert looper.is_recording, "Should still be recording"

    sequencer._run_tick()
    assert not looper.is_recording, "Should have stopped recording"

@patch('time.sleep')
def test_overdub_recording_duration(mock_sleep):
    """
    Tests that overdub recording uses the correct duration in BEATS.
    """
    bpm = 120
    midi_queue = queue.Queue()
    gui_queue = queue.Queue()
    looper = LooperModule(gui_queue=gui_queue)
    looper.set_bpm(bpm)
    sequencer = Sequencer(midi_queue, gui_queue=gui_queue, looper=looper, bpm=bpm)
    looper.set_sequencer(sequencer)
    sequencer.play()

    loop_notes = [{'pitch': 60, 'velocity': 100, 'start': 0.0, 'end': 2.0}]
    looper.load_loop(notes=loop_notes, bpm=bpm)

    looper.start_recording(record_length_beats=999, start_beat=0.0, overdub=True)

    assert looper.record_length_beats == pytest.approx(4.0)

    for _ in range(1001):
        sequencer._run_tick()
    assert not looper.is_recording

@patch('time.sleep')
def test_looper_state_reset_on_recapture(mock_sleep):
    """
    Tests that the looper's playback state is reset on recapture.
    """
    bpm = 120
    midi_queue = queue.Queue()
    gui_queue = queue.Queue()
    looper = LooperModule(gui_queue=gui_queue)
    looper.set_bpm(bpm)
    sequencer = Sequencer(midi_queue, gui_queue=gui_queue, looper=looper, bpm=bpm)
    looper.set_sequencer(sequencer)
    sequencer.play()

    loop1_notes = [{'pitch': 60, 'velocity': 100, 'start': 0.0, 'end': 1.0}]
    looper.load_loop(notes=loop1_notes, bpm=bpm)
    looper.play()

    for _ in range(1250):
        sequencer._run_tick()
    assert looper.loop_cycle_count >= 2

    new_loop_notes = [{'pitch': 62, 'velocity': 100, 'start': 0.0, 'end': 0.25}]
    looper.capture_seed(notes=new_loop_notes, scale_notes=[], root_note=60, scale_degrees=[], bpm=bpm, duration_beats=1.0)

    assert looper.loop_cycle_count == 0

    sequencer._run_tick()
    note_off_events = [e for e in sequencer.scheduled_events if e[2].type == 'note_off' and e[2].note == 62]
    assert len(note_off_events) == 1
    off_time, _, _, _ = note_off_events[0]
    assert off_time == pytest.approx(5.25, abs=1e-2)

def test_note_off_scheduling_is_monotonic():
    """
    Tests that note_off events are scheduled in a strictly increasing
    order of beats, which prevents hanging notes.
    """
    midi_queue = queue.Queue()
    looper = LooperModule()
    sequencer = Sequencer(midi_queue, looper=looper, bpm=120)
    looper.set_sequencer(sequencer)
    looper.set_bpm(120)

    scheduled_off_times = []
    def mock_schedule_note_off(pitch, off_beat, owner='looper'):
        scheduled_off_times.append(off_beat)

    sequencer.schedule_external_note_off = mock_schedule_note_off

    loop_notes = [
        {'pitch': 60, 'velocity': 100, 'start': 0.0, 'end': 0.5},
        {'pitch': 62, 'velocity': 100, 'start': 1.0, 'end': 1.5},
    ]
    looper.load_loop(notes=loop_notes, bpm=120)

    sequencer.play()
    looper.play()

    with patch('time.sleep'):
        for _ in range(3750):
            sequencer._run_tick()

    assert len(scheduled_off_times) > 5, "Should have scheduled several note_offs"
    assert scheduled_off_times == sorted(scheduled_off_times), "Note-off times should be monotonically increasing"
