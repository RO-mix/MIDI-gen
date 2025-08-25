import threading
import time
import queue
import mido
import bisect
import logging
from src.theory.scales import get_notes_in_scale, SCALES

class Sequencer(threading.Thread):
    """
    A MIDI event scheduler that runs in a dedicated thread. It generates notes
    from a given generator and schedules their note_off events, allowing for
    variable note durations and overlapping notes.
    """
    def __init__(self, midi_queue: queue.Queue, gui_queue: queue.Queue = None, command_queue: queue.Queue = None, bpm: int = 120, looper=None):
        super().__init__()
        self.logger = logging.getLogger(__name__)
        self.midi_queue = midi_queue
        self.gui_queue = gui_queue
        self.command_queue = command_queue
        self.daemon = True
        self.looper = looper

        # Threading events
        self._running = threading.Event()
        self._playing = threading.Event()

        # Sequencer state
        self.bpm = bpm
        self.midi_channel = 0
        self.generator = None
        self.scheduled_events = []  # A sorted list of (off_time, event_id, message)
        self.current_beat = 0.0
        self.next_note_beat = 0.0
        self.event_counter = 0 # A unique ID for each event to ensure stable sorting
        self.last_tick_time = 0.0
        self.active_notes = set() # Keep track of notes that are currently on
        self.is_muted = False
        self.last_range_reset_beat = 0.0
        self.last_beat_sent_time = 0.0
        self.TICK_INTERVAL_S = 0.002  # 2ms for a 500Hz tick rate
        self.scheduled_commands = [] # (beat, command, args)

    def run(self):
        """
        The main high-resolution ticker loop for the sequencer. This loop is
        responsible for advancing the master clock (`current_beat`) and calling
        the `_tick` method to process events for that beat.
        """
        self._running.set()
        print("Sequencer thread started.")

        while self._running.is_set():
            try:
                self._run_tick()
            except Exception:
                self.logger.error("Unhandled exception in Sequencer thread, stopping.", exc_info=True)
                self._running.clear() # Stop the thread on error

        print("Sequencer thread stopped.")

    def _run_tick(self):
        """Performs one iteration of the main run loop. Extracted for testability."""
        start_time = time.perf_counter()

        if self._playing.is_set():
            # Calculate how many beats have passed in this tick
            beats_per_second = self.bpm / 60.0
            delta_beats = self.TICK_INTERVAL_S * beats_per_second
            self.current_beat += delta_beats

            # Process all events for the new current_beat
            self._tick()

        # Smart sleep to maintain a consistent tick rate
        end_time = time.perf_counter()
        sleep_time = self.TICK_INTERVAL_S - (end_time - start_time)
        if sleep_time > 0:
            time.sleep(sleep_time)

    def _tick(self):
        """
        Performs a single time step of the sequencer logic. It processes all
        events that should occur at the current value of `self.current_beat`.
        This method does NOT advance time itself.
        """
        current_time = time.perf_counter() # Still needed for GUI updates

        # --- Process Scheduled Commands ---
        while self.scheduled_commands and self.scheduled_commands[0][0] <= self.current_beat:
            _, command, args = self.scheduled_commands.pop(0)
            self._execute_command(command, args)


        # --- Process Scheduled Note-Off Events ---
        while self.scheduled_events and self.scheduled_events[0][0] <= self.current_beat:
            _, _, msg, owner = self.scheduled_events.pop(0)
            bypass_mute = (owner == 'looper')
            self._send_message(msg, bypass_mute=bypass_mute)
            if msg.type == 'note_off':
                self.active_notes.discard(msg.note)

        # --- Generate New Notes ---
        if self.generator and self.current_beat >= self.next_note_beat:
            events, duration_to_next_event = self.generator.generate(self.current_beat)

            for msg, duration_beats in events:
                msg.channel = self.midi_channel
                self._send_message(msg)
                if msg.type == 'note_on':
                    self.active_notes.add(msg.note)
                    off_time = self.current_beat + duration_beats
                    note_off_msg = mido.Message('note_off', note=msg.note, channel=self.midi_channel)
                    bisect.insort(self.scheduled_events, (off_time, self.event_counter, note_off_msg, 'generator'))
                    self.event_counter += 1

                    note_info = {
                        'pitch': msg.note,
                        'velocity': msg.velocity,
                        'start_beat': self.current_beat,
                        'duration_beats': duration_beats
                    }

                    if self.gui_queue:
                        self.gui_queue.put(('live_note', note_info))

                    if self.looper:
                        self.looper.add_live_note(note_info)

            if duration_to_next_event > 0:
                self.next_note_beat += duration_to_next_event
            else:
                self.next_note_beat += 1

        if self.gui_queue:
            if self.current_beat - self.last_range_reset_beat >= 8.0:
                self.gui_queue.put(('reset_range',))
                self.last_range_reset_beat = self.current_beat

        if self.looper:
            self.looper.tick(self.current_beat)

        self._process_commands()

    def _get_next_quantized_beat(self, quantize_mode: str) -> float:
        """Calculates the target beat based on the selected quantization mode."""
        if quantize_mode == "instant":
            # Return a beat slightly in the future to ensure it's processed in the next tick
            return self.current_beat + 0.001
        if quantize_mode == "beat":
            return float(int(self.current_beat) + 1)
        if quantize_mode == "half_beat":
            return (int(self.current_beat * 2) + 1) / 2.0
        # Default to bar quantization ("bar")
        beats_per_bar = 4
        current_bar_number = self.current_beat // beats_per_bar
        next_bar_start_beat = (current_bar_number + 1) * beats_per_bar
        return next_bar_start_beat

    def _process_commands(self):
        """Pulls commands from the GUI queue and decides whether to execute now or schedule."""
        if not self.command_queue:
            return
        try:
            command, args = self.command_queue.get_nowait()
            self.logger.debug(f"Sequencer received command: {command} with args {args}")

            action_quantize = args.get('quantize', 'bar')

            # For commands that need to be scheduled, calculate the target beat and schedule them.
            # For others that can happen instantly, execute them.
            if action_quantize == 'instant':
                self._execute_command(command, args)
            else:
                target_beat = self._get_next_quantized_beat(action_quantize)
                # Using bisect to keep the list sorted by beat
                bisect.insort(self.scheduled_commands, (target_beat, command, args))
                self.logger.debug(f"Scheduled command {command} for beat {target_beat}")

        except queue.Empty:
            return
        except Exception as e:
            self.logger.error(f"Error processing command: {e}", exc_info=True)

    def _execute_command(self, command, args):
        """Contains the actual logic for executing a command."""
        self.logger.debug(f"Executing command: {command} at beat {self.current_beat}")

        if command == 'looper_capture':
            is_overdub = args.get('is_overdub', False)

            root_note = args.get('root_note', 0)
            scale_name = args.get('scale_name', 'Major')
            scale_degrees = SCALES.get(scale_name, [0, 2, 4, 5, 7, 9, 11])
            scale_notes = get_notes_in_scale(root_note, scale_degrees)

            captured_notes = self.generator.capture(start_beat=self.current_beat, duration_beats=args['duration_beats'])

            if is_overdub:
                self.looper.overdub_with_notes(
                    notes=captured_notes,
                    duration_beats=args['duration_beats']
                    )
            else:
                self.looper.capture_seed(
                    notes=captured_notes,
                    scale_notes=scale_notes,
                    root_note=root_note,
                    scale_degrees=scale_degrees,
                    bpm=self.bpm,
                    duration_beats=args['duration_beats']
                )

            if not self.looper.is_playing():
                self.looper.play()
                if not args.get('is_through', False):
                    self.set_muted(True)

        elif command == 'looper_toggle_record':
            if self.looper.is_recording:
                # This is a STOP RECORD action
                self.looper.stop_recording()
                # After stopping, always unmute so we can hear the result or the generator
                if self.is_muted:
                    self.set_muted(False)
            else:
                # This is a START RECORD action
                is_overdub = args.get('is_overdub', False)
                is_through = args.get('is_through', False)

                # If it's a new recording (not overdub), we might mute the generator.
                # If it's an overdub, we never want to mute the generator.
                if not is_overdub:
                    self.set_muted(not is_through)

                self.looper.start_recording(
                    record_length_beats=args['length_beats'],
                    start_beat=self.current_beat,
                    overdub=is_overdub
                )

        elif command == 'looper_toggle_play':
             if self.looper.is_playing():
                 self.looper.stop()
                 if self.is_muted:
                    self.set_muted(False)
             else:
                 self.looper.play(start_beat=self.current_beat) # Play starts now
                 if not args.get('is_through', False):
                     self.set_muted(True)

        elif command == 'looper_schedule_split':
            self.looper.schedule_split()

        elif command == 'looper_double':
            self.looper.double_loop()

    def play_looper_note(self, msg):
        """Plays a note from the looper, bypassing the generator mute."""
        self.midi_queue.put(msg)
        if self.gui_queue and msg.type == 'note_on':
            # Use a different blinker or no blinker for looper notes to avoid confusion.
            # For now, let's not blink for looper notes.
            pass

    def _send_message(self, msg, bypass_mute=False):
        """Puts a message on the MIDI and GUI queues if not muted."""
        if self.is_muted and not bypass_mute:
            return

        self.midi_queue.put(msg)
        if self.gui_queue and msg.type == 'note_on':
            self.gui_queue.put("blink")

    def schedule_external_note_off(self, note_pitch, off_beat, owner='looper'):
        note_off_msg = mido.Message('note_off', note=note_pitch, channel=self.midi_channel)
        bisect.insort(self.scheduled_events, (off_beat, self.event_counter, note_off_msg, owner))
        self.event_counter += 1

    def set_muted(self, muted: bool):
        self.is_muted = muted
        if self.is_muted:
            self._flush_all_notes()
        print(f"Sequencer muted: {self.is_muted}")

    def _flush_all_notes(self):
        for note in list(self.active_notes):
            msg = mido.Message('note_off', note=note, channel=self.midi_channel)
            self._send_message(msg, bypass_mute=True)
        self.active_notes.clear()

    def cancel_all_events(self):
        self.scheduled_events.clear()
        print("Cancelled all scheduled events.")

    def cancel_events_by_owner(self, owner_to_cancel):
        self.scheduled_events = [event for event in self.scheduled_events if event[3] != owner_to_cancel]
        print(f"Cancelled events for owner: {owner_to_cancel}. Remaining events: {len(self.scheduled_events)}")

    def set_midi_channel(self, channel: int):
        if 0 <= channel < 16:
            self.midi_channel = channel

    def stop(self):
        self._flush_all_notes()
        self.cancel_all_events()
        self._running.clear()

    def play(self):
        self.last_tick_time = time.perf_counter()
        self._playing.set()
        print("Sequencer playback started.")

    def pause(self):
        self._playing.clear()
        self._flush_all_notes()
        self.cancel_all_events()
        print("Sequencer playback paused.")

    def is_playing(self):
        return self._playing.is_set()

    def panic(self):
        self.logger.info("PANIC: Sending all notes off and clearing queues.")
        for channel in range(16):
            for note in range(128):
                self.midi_queue.put(mido.Message('note_off', note=note, channel=channel, velocity=0))
        self.active_notes.clear()
        self.cancel_all_events()

    def set_bpm(self, bpm: int):
        if bpm > 0:
            self.bpm = bpm

    def set_generator(self, generator):
        self.generator = generator
