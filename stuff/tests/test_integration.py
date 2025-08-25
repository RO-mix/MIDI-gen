import queue
import mido
import pytest

from src.engine.sequencer import Sequencer
from src.looper.looper_module import LooperModule
from src.generators.random_generator import RandomGenerator


def test_capture_produces_content():
    """
    A high-level test to ensure the generator's capture method
    actually produces note data.
    """
    generator = RandomGenerator()
    # Ensure the generator is likely to produce notes
    generator.update_params(note_probability=1.0, rate=1.0)

    # Capture for 4 beats
    captured_notes = generator.capture(duration_beats=4.0, start_beat=0.0)

    assert len(captured_notes) > 0, "Generator capture returned an empty list."


def test_recording_produces_content():
    """
    A high-level test to ensure the full recording pipeline
    (sequencer -> looper) results in recorded notes.
    """
    # 1. Setup
    midi_queue = queue.Queue()
    gui_queue = queue.Queue()
    looper = LooperModule(gui_queue=gui_queue)
    sequencer = Sequencer(midi_queue, gui_queue=gui_queue, looper=looper, bpm=120)
    looper.set_sequencer(sequencer)
    looper.set_bpm(120)

    # 2. Configure a generator that is guaranteed to produce notes
    generator = RandomGenerator()
    generator.update_params(note_probability=1.0, rate=1.0)
    sequencer.set_generator(generator)

    # 3. Start recording
    sequencer.play()
    looper.start_recording(record_length_beats=4.0, start_beat=sequencer.current_beat)

    # 4. Run the sequencer for 4 beats
    # 4 beats at 120 bpm = 2s. 2s / 0.002s/tick = 1000 ticks.
    for _ in range(1001):
        sequencer._run_tick()

    # 5. Assertions
    # The looper should have stopped recording automatically via its tick method.
    assert not looper.is_recording
    # The recorded_notes buffer should have content.
    assert len(looper.recorded_notes) > 0, "Looper recorded_notes list is empty after recording."


def test_long_note_truncation_and_note_off():
    """
    Tests that a long note crossing the capture boundary is correctly
    truncated and that its note_off event is scheduled for the end of the loop.
    This directly tests the user's hypothesis for the hanging notes bug.
    """
    # 1. Setup
    midi_queue = queue.Queue()
    gui_queue = queue.Queue()
    looper = LooperModule(gui_queue=gui_queue)
    sequencer = Sequencer(midi_queue, gui_queue=gui_queue, looper=looper, bpm=120)
    looper.set_sequencer(sequencer)
    looper.set_bpm(120)

    # 2. Configure a generator to produce one very long note
    class LongNoteGenerator(RandomGenerator):
        def capture(self, duration_beats, start_beat=0.0):
            # Return one note that starts at beat 1 and lasts for 10 beats
            return [{'pitch': 60, 'velocity': 100, 'start': 1.0, 'end': 11.0}]
        def generate(self, current_beat):
            # Do not generate any notes during regular playback for this test
            return ([], 1.0)

    sequencer.set_generator(LongNoteGenerator())

    # 3. Capture for 4 beats (replicating logic from MainWindow._do_capture)
    capture_duration = 4.0
    captured_notes = sequencer.generator.capture(duration_beats=capture_duration)
    looper.capture_seed(
        notes=captured_notes,
        scale_notes=[60],
        root_note=60,
        scale_degrees=[0],
        bpm=120,
        duration_beats=capture_duration
    )

    # 4. Start BOTH the looper and the sequencer
    looper.play()
    sequencer.play() # THIS WAS THE MISSING PIECE

    # 5. Run the sequencer's main loop to advance time and process events
    # Test runs for ~10 beats.
    for _ in range(2501):
        sequencer._run_tick()

    # 6. Assertions
    # The loop's duration should be 4.0 beats.
    assert looper.loop_duration_beats == pytest.approx(4.0)

    # The single note in the loop should be truncated to the loop duration
    assert len(looper.loop) == 1
    assert looper.loop[0]['end_beat'] == pytest.approx(4.0)

    # At the end of the run, there should be no *overdue* events. Future events from the loop are okay.
    final_scheduled_events = [e for e in sequencer.scheduled_events if e[2].type == 'note_off']
    overdue_events = [e for e in final_scheduled_events if e[0] <= sequencer.current_beat]
    assert len(overdue_events) == 0, "There should be no overdue note_off events left in the schedule."

    # The most important check: were the note_off messages sent to the queue?
    # The loop is 4 beats, the test runs for ~10 beats. The note should play at ~0 and ~4, so 2 note_offs.
    note_off_msgs = [m for m in list(midi_queue.queue) if m.type == 'note_off']
    assert len(note_off_msgs) >= 2, "At least two note_off messages should have been sent to the queue."
    assert note_off_msgs[0].note == 60
