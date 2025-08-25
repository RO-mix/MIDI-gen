import unittest
from unittest.mock import Mock, patch
import queue
import time

import sys
import os

# Add the src directory to the Python path
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

from src.engine.sequencer import Sequencer
from src.looper.looper_module import LooperModule
from src.generators.base import BaseGenerator

class MockGenerator(BaseGenerator):
    """A mock generator that returns a predefined set of notes."""
    def __init__(self, notes_to_return):
        super().__init__()
        self._notes = notes_to_return

    def generate(self, start_beat):
        # Returns the notes and a duration to the next event (e.g., 1 beat)
        return self._notes, 1.0

    def capture(self, start_beat, duration_beats):
        # Return notes (which are dicts) within the capture window
        return [n for n in self._notes if start_beat <= n.get('start', 0) < start_beat + duration_beats]


class TestSequencerLooperIntegration(unittest.TestCase):

    def setUp(self):
        self.midi_queue = queue.Queue()
        self.gui_queue = queue.Queue()
        self.command_queue = queue.Queue()
        self.looper = LooperModule(gui_queue=self.gui_queue)
        self.sequencer = Sequencer(self.midi_queue, self.gui_queue, self.command_queue, looper=self.looper)
        self.looper.set_sequencer(self.sequencer)

        # We will manually drive the sequencer's clock
        self.sequencer._playing.set()


    def test_quantized_split_action(self):
        """Test that a 'split' command is scheduled and executed on the next bar."""
        # 1. Setup: Give the looper a basic loop
        initial_notes = [{'pitch': 60, 'velocity': 100, 'start': 0.0, 'end': 1.0}]
        self.looper.capture_seed(notes=initial_notes, scale_notes=[], root_note=60, scale_degrees=[], bpm=120, duration_beats=4.0)
        self.assertEqual(len(self.looper.loop), 1)

        # 2. Action: Put a 'looper_schedule_split' command on the queue, quantized to the next bar
        self.sequencer.current_beat = 1.2 # Somewhere in the first bar
        args = {'quantize': 'bar'}
        self.command_queue.put(('looper_schedule_split', args))

        # 3. Execution: Process the command
        self.sequencer._process_commands()

        # 4. Assert: The command should be scheduled, not executed yet
        self.assertEqual(len(self.sequencer.scheduled_commands), 1)
        self.assertEqual(self.sequencer.scheduled_commands[0][1], 'looper_schedule_split')
        self.assertAlmostEqual(self.sequencer.scheduled_commands[0][0], 4.0) # Next bar starts at beat 4.0

        # Mock the looper's split method to see if it gets called
        self.looper.schedule_split = Mock()

        # 5. Advance time just before the scheduled beat
        self.sequencer.current_beat = 3.99
        self.sequencer._tick()
        self.looper.schedule_split.assert_not_called()

        # 6. Advance time to the scheduled beat
        self.sequencer.current_beat = 4.0
        self.sequencer._tick()
        self.looper.schedule_split.assert_called_once()
        self.assertEqual(len(self.sequencer.scheduled_commands), 0) # Command should be consumed


    def test_overdub_on_capture_feature(self):
        """Test that capturing with the overdub flag adds notes instead of replacing."""
        # 1. Setup: Give the looper an initial loop with one note
        initial_notes = [{'pitch': 60, 'velocity': 100, 'start': 0.0, 'end': 1.0}]
        self.looper.capture_seed(notes=initial_notes, scale_notes=[], root_note=60, scale_degrees=[], bpm=120, duration_beats=4.0)
        self.assertEqual(len(self.looper.loop), 1)

        # 2. Setup a mock generator for the sequencer to "capture" from
        notes_to_capture = [{'pitch': 62, 'velocity': 90, 'start': 0.5, 'end': 1.5}]
        self.sequencer.set_generator(MockGenerator(notes_to_capture))

        # 3. Action: Send the capture command with is_overdub = True
        args = {
            'quantize': 'instant',
            'duration_beats': 4.0,
            'is_overdub': True,
            'root_note': 60,
            'scale_name': 'Major'
        }
        self.command_queue.put(('looper_capture', args))

        # 4. Execution: Let the sequencer process the command
        self.sequencer._process_commands()

        # 5. Assert: The loop should now have two notes
        self.assertEqual(len(self.looper.loop), 2)
        self.assertEqual(self.looper.loop[0]['pitch'], 60)
        self.assertEqual(self.looper.loop[1]['pitch'], 62)


    def test_recording_and_playback_sync(self):
        """
        Test that a recorded loop stays in sync with the sequencer's master beat.
        This verifies the core fix of the beat-native refactoring.
        """
        # 1. Setup: Start recording
        record_beats = 4.0
        self.sequencer._execute_command('looper_toggle_record', {'is_overdub': False, 'length_beats': record_beats})
        self.assertTrue(self.looper.is_recording)

        # 2. Action: Add some notes during the recording window
        # Note starts are absolute beats from the sequencer's perspective
        self.looper.add_live_note({'pitch': 60, 'start_beat': 0.5, 'duration_beats': 0.4, 'velocity': 100})
        self.looper.add_live_note({'pitch': 62, 'start_beat': 1.5, 'duration_beats': 0.4, 'velocity': 100})

        # 3. Stop recording
        self.sequencer.current_beat = record_beats
        self.sequencer._tick() # Tick should trigger the auto-stop
        self.assertFalse(self.looper.is_recording)
        self.assertIsNotNone(self.looper.loop)
        self.assertEqual(len(self.looper.loop), 2)
        # The notes should have been normalized to start at 0.5 and 1.5 within the loop
        self.assertAlmostEqual(self.looper.loop[0]['start_beat'], 0.5)
        self.assertAlmostEqual(self.looper.loop[1]['start_beat'], 1.5)

        # 4. Execution: Start playback and run for several cycles
        self.looper.play(start_beat=self.sequencer.current_beat)

        self.looper.sequencer.play_looper_note = Mock()

        # --- First cycle ---
        # Trigger first note
        self.sequencer.current_beat = 4.51 # Loop started at 4.0, note is at 0.5
        self.looper.tick(self.sequencer.current_beat)
        self.looper.sequencer.play_looper_note.assert_called_once()
        # Trigger second note
        self.sequencer.current_beat = 5.51 # Loop started at 4.0, note is at 1.5
        self.looper.tick(self.sequencer.current_beat)
        self.assertEqual(self.looper.sequencer.play_looper_note.call_count, 2)

        # --- Second cycle (Loop is 4 beats long) ---
        self.looper.sequencer.play_looper_note.reset_mock()
        # Trigger first note again
        self.sequencer.current_beat = 8.51 # 4.0 (start) + 4.0 (cycle) + 0.5 (note)
        self.looper.tick(self.sequencer.current_beat)
        self.looper.sequencer.play_looper_note.assert_called_once()
        # Trigger second note again
        self.sequencer.current_beat = 9.51 # 4.0 (start) + 4.0 (cycle) + 1.5 (note)
        self.looper.tick(self.sequencer.current_beat)
        self.assertEqual(self.looper.sequencer.play_looper_note.call_count, 2)

if __name__ == '__main__':
    unittest.main()
