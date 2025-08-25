import pytest
import queue
import time
import mido
from src.engine.sequencer import Sequencer
from src.generators.base import BaseGenerator

# A mock generator for testing purposes
class MockGenerator(BaseGenerator):
    def __init__(self):
        self.called = 0
        self.note = 60
        self.duration = 1.0
        self.velocity = 100
        self.channel = 0

    def generate(self, current_beat):
        self.called += 1
        msg = mido.Message('note_on', note=self.note, velocity=self.velocity, channel=self.channel)
        # The new signature requires returning a tuple: (list_of_events, duration_to_next_event)
        # For this mock, we'll return a fixed duration_to_next_event of 1.0 beat.
        return ([(msg, self.duration)], 1.0)

    def generate_empty(self, current_beat):
        self.called += 1
        return ([], 1.0)


@pytest.fixture
def midi_queue():
    return queue.Queue()

@pytest.fixture
def gui_queue():
    return queue.Queue()

@pytest.fixture
def sequencer(midi_queue, gui_queue):
    """Fixture to create a Sequencer instance and ensure it's stopped after the test."""
    seq = Sequencer(midi_queue, gui_queue)
    yield seq
    # Cleanup: ensure the thread is stopped even if a test fails
    if seq.is_alive():
        seq.stop()
        seq.join()

def test_sequencer_initializes_in_stopped_state_with_defaults(sequencer):
    assert sequencer.bpm == 120
    assert sequencer.generator is None
    assert not sequencer.is_playing()

def test_set_bpm_updates_bpm_value(sequencer):
    sequencer.set_bpm(140)
    assert sequencer.bpm == 140

def test_set_generator_updates_generator_instance(sequencer):
    gen = MockGenerator()
    sequencer.set_generator(gen)
    assert sequencer.generator == gen

def test_play_and_pause_correctly_set_playing_state(sequencer):
    assert not sequencer.is_playing()
    sequencer.play()
    assert sequencer.is_playing()
    sequencer.pause()
    assert not sequencer.is_playing()

def test_tick_generates_note_and_schedules_note_off(mocker, sequencer, midi_queue):
    """
    Test that the sequencer generates a note and schedules its note_off.
    """
    # Mock time to control the sequencer loop
    mocker.patch('time.perf_counter', side_effect=[100.0, 100.1])

    gen = MockGenerator()
    gen.duration = 1.5 # beats
    sequencer.set_generator(gen)
    sequencer.set_bpm(120) # 2 beats per second

    sequencer.play()

    # Let the sequencer run for one "tick"
    sequencer._tick() # We call _tick() directly to control execution

    # Check that the generator was called
    assert gen.called == 1

    # Check that a note_on message was sent
    note_on_msg = midi_queue.get_nowait()
    assert note_on_msg.type == 'note_on'
    assert note_on_msg.note == 60

    # Check that a note_off message was scheduled correctly
    assert len(sequencer.scheduled_events) == 1
    off_time, _, note_off_msg, owner = sequencer.scheduled_events[0] # Unpack 4-tuple

    # current_beat advances before generation, so off_time includes this advancement.
    # The _tick() method doesn't advance time, so current_beat is 0.0
    # off_time = current_beat (0.0) + duration (1.5) = 1.5
    assert off_time == pytest.approx(1.5)
    assert note_off_msg.type == 'note_off'
    assert note_off_msg.note == 60
    assert owner == 'generator'

def test_tick_sends_scheduled_note_off_when_beat_is_reached(mocker, sequencer, midi_queue):
    """
    Test that a scheduled note_off event is sent at the correct time.
    """
    sequencer.set_bpm(60) # 1 beat per second
    sequencer.current_beat = 0.9

    # Manually schedule a note_off event at beat 1.0
    note_off_msg = mido.Message('note_off', note=72)
    # Append a 4-tuple to match the new event format (time, id, msg, owner)
    sequencer.scheduled_events.append((1.0, 0, note_off_msg, 'test'))
    sequencer.active_notes.add(72)

    # Mock time to advance the beat past the note_off time.
    mocker.patch('time.perf_counter', side_effect=[100.0, 101.0])

    sequencer.play()    # Consumes one value to set last_tick_time

    # Manually advance the beat past the event time and then tick
    sequencer.current_beat = 1.1
    sequencer._tick()

    # The current_beat is what we set it to
    assert sequencer.current_beat == pytest.approx(1.1)

    # The note_off message should have been sent
    assert midi_queue.qsize() == 1
    sent_msg = midi_queue.get_nowait()
    assert sent_msg == note_off_msg

    # The event and active note should be cleared
    assert not sequencer.scheduled_events
    assert not sequencer.active_notes

def test_pause_sends_note_off_for_all_active_notes(sequencer, midi_queue):
    """
    Test that pausing sends note_off for all active notes.
    """
    sequencer.active_notes.add(60)
    sequencer.active_notes.add(72)

    sequencer.pause()

    assert midi_queue.qsize() == 2
    messages = [midi_queue.get(), midi_queue.get()]
    notes = {msg.note for msg in messages}
    assert notes == {60, 72}
    assert all(msg.type == 'note_off' for msg in messages)
    assert not sequencer.active_notes
