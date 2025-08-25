import pretty_midi
import mido
import threading
import time
import random
import copy
import logging

class LooperModule:
    """
    An intelligent MIDI looper that captures a seed from a generator,
    analyzes it, and applies musical variations to create an evolving loop.
    """
    def __init__(self, gui_queue=None):
        self.logger = logging.getLogger(__name__)

        # --- Core Loop Data (Beat-native) ---
        # self.loop and self.pristine_loop are lists of note dicts:
        # [{'pitch': p, 'velocity': v, 'start_beat': s, 'end_beat': e}]
        self.loop = None
        self.pristine_loop = None
        self.loop_duration_beats = 0.0
        self.is_quantized = False
        self.bpm = 120

        # --- Musical Context ---
        self.scale_notes = []  # Full list of MIDI note numbers in the scale
        self.scale_degrees = [] # e.g., [0, 2, 4, 5, 7, 9, 11] for Major
        self.root_note = 0

        # --- Variation Parameters ---
        self.intensity = {'bass': 0.0, 'mid': 0.0, 'high': 0.0}
        self._analysis_results = {}

        # --- Playback State ---
        self._playing = threading.Event()
        self.loop_start_beat = 0.0
        self.last_tick_beat = 0.0
        self.loop_cycle_count = 0
        self._is_first_tick = True # Flag to handle the first tick after play
        self.auto_recapture_period = 0  # 0 means off
        self.loop_lock = threading.Lock()
        self.active_looper_notes = set()
        self.played_notes_this_cycle = set()
        self.split_is_scheduled = False
        self.pad_mode = False

        # --- Recording State ---
        self.is_recording = False
        self.is_overdubbing = False
        self.is_pending_replace = False
        self.was_playing_before_record = False
        self.recording_start_beat = 0.0
        self.record_length_beats = 0.0
        self.recorded_notes = [] # Also beat-native

        # --- Communication ---
        self.gui_queue = gui_queue
        self.sequencer = None

    def set_sequencer(self, sequencer):
        """Provides the looper with a reference to the main sequencer."""
        self.sequencer = sequencer

    def set_pad_mode(self, is_on: bool):
        """Enables or disables Pad Mode."""
        self.logger.info(f"Setting pad mode to: {is_on}")
        self.pad_mode = is_on
        if not self.pad_mode:
            self.logger.info("Pad mode disabled, flushing all notes.")
            self._flush_all_notes()

    def set_bpm(self, bpm: int):
        """Updates the BPM for the looper's internal calculations."""
        self.bpm = bpm

    def capture_seed(self, notes: list[dict], scale_notes: list[int], root_note: int, scale_degrees: list[int], bpm: int, duration_beats: float):
        """
        Captures a seed of notes from a generator and sets up the initial loop.
        Notes are expected to have 'start' and 'end' in beats.
        """
        self.logger.info(f"Capturing seed with {len(notes)} notes for {duration_beats} beats.")

        self.logger.debug("Flushing notes and cancelling events before capture.")
        self._flush_all_notes()
        if self.sequencer:
            self.sequencer.cancel_events_by_owner('looper')

        if not notes:
            self.logger.warning("Captured seed is empty. Aborting.")
            self.loop = None
            self.pristine_loop = None
            self.loop_duration_beats = 0.0
            return

        self.bpm = bpm
        self.scale_notes = sorted(list(set(scale_notes)))
        self.root_note = root_note
        self.scale_degrees = scale_degrees

        min_start_beat = min((n['start'] for n in notes), default=0)

        new_loop_notes = []
        for n in notes:
            # Normalize to start at beat 0
            start_beat = n['start'] - min_start_beat
            end_beat = n['end'] - min_start_beat

            # Truncate notes that extend beyond the capture duration
            start_beat = max(0, start_beat)
            end_beat = min(end_beat, duration_beats)

            if start_beat < end_beat: # Ensure note has positive duration
                new_loop_notes.append({
                    'pitch': n['pitch'],
                    'velocity': n['velocity'],
                    'start_beat': start_beat,
                    'end_beat': end_beat
                })

        with self.loop_lock:
            self.loop = new_loop_notes
            self.pristine_loop = copy.deepcopy(self.loop)
            self.loop_duration_beats = duration_beats
            self.is_quantized = False
            self.pad_mode = False
            self._analyze_loop()
            self._reset_playback_state()

    def overdub_with_notes(self, notes: list[dict], duration_beats: float):
        """
        Overdubs a list of notes onto the existing loop.
        If no loop exists, this acts like a regular capture.
        """
        if not self.loop:
            self.logger.info("No existing loop for overdub, capturing as a new loop.")
            # Fallback to capture_seed behavior. We need to get the scale info,
            # but it should already be set from previous operations.
            self.capture_seed(notes, self.scale_notes, self.root_note, self.scale_degrees, self.bpm, duration_beats)
            return

        self.logger.info(f"Overdubbing with {len(notes)} notes.")
        min_start_beat = min((n['start'] for n in notes), default=0)

        with self.loop_lock:
            # Normalize and add the new notes
            for n in notes:
                start_beat = n['start'] - min_start_beat
                end_beat = n['end'] - min_start_beat
                if start_beat < end_beat and end_beat <= self.loop_duration_beats:
                    self.loop.append({
                        'pitch': n['pitch'],
                        'velocity': n['velocity'],
                        'start_beat': start_beat,
                        'end_beat': end_beat
                    })

            # The loop is now modified, so the pristine version must be updated.
            self.pristine_loop = copy.deepcopy(self.loop)
            self.is_quantized = False
            self._analyze_loop()
            # We don't reset playback state, so the overdub happens seamlessly.

    def _analyze_loop(self):
        """
        Analyzes the current loop (list of note dicts) to extract structural information.
        NOTE: This method should be called within a `loop_lock` context.
        """
        if not self.loop:
            self._analysis_results = {'pitch_histogram': {}, 'avg_duration_beats': 0, 'note_density': 0}
            return

        notes = self.loop
        if not notes:
            self._analysis_results = {'pitch_histogram': {}, 'avg_duration_beats': 0, 'note_density': 0}
            return

        pitch_histogram = {}
        for note in notes:
            pitch_histogram[note['pitch']] = pitch_histogram.get(note['pitch'], 0) + 1

        avg_duration_beats = sum(n['end_beat'] - n['start_beat'] for n in notes) / len(notes) if notes else 0

        note_density = len(notes) / self.loop_duration_beats if self.loop_duration_beats > 0 else 0

        self._analysis_results = {
            'pitch_histogram': pitch_histogram,
            'avg_duration_beats': avg_duration_beats,
            'note_density': note_density
        }
        self.logger.debug(f"Loop analysis complete: {self._analysis_results}")

    def double_loop(self):
        """Doubles the duration of the loop by duplicating all notes."""
        self._flush_all_notes()
        if self.sequencer:
            self.sequencer.cancel_events_by_owner('looper')
        self._reset_playback_state()

        with self.loop_lock:
            if not self.loop:
                self.logger.warning("Cannot double loop: loop is empty.")
                return

            self.logger.info("Doubling loop duration.")

            original_notes = copy.deepcopy(self.loop)
            duration = self.loop_duration_beats
            if duration <= 0:
                self.logger.warning("Cannot double loop: loop has no duration.")
                return

            for note in original_notes:
                self.loop.append({
                    'pitch': note['pitch'],
                    'velocity': note['velocity'],
                    'start_beat': note['start_beat'] + duration,
                    'end_beat': note['end_beat'] + duration
                })

            self.loop_duration_beats *= 2
            self.pristine_loop = copy.deepcopy(self.loop)
            self.is_quantized = False
            self._analyze_loop()

    def generate_variations(self):
        """
        Generates intelligent variations on the current loop based on intensity settings.
        This operation replaces the current loop with a new, varied one.
        """
        self._flush_all_notes()
        if self.sequencer:
            self.sequencer.cancel_events_by_owner('looper')
        self._reset_playback_state()

        with self.loop_lock:
            if not self.loop or not self.scale_notes:
                self.logger.warning("Cannot generate variations: loop or scale is missing.")
                return

            self.logger.info(f"Generating variations with intensity: {self.intensity}")

            varied_loop = copy.deepcopy(self.loop)
            notes_to_add = []

            for note in self.loop:
                # Bass variations
                if note['pitch'] < 48 and random.random() < self.intensity['bass']:
                    new_pitch = note['pitch'] - 12
                    if new_pitch in self.scale_notes:
                        notes_to_add.append({'pitch': new_pitch, 'velocity': note['velocity'], 'start_beat': note['start_beat'], 'end_beat': note['end_beat']})
                    # Vary duration
                    duration = note['end_beat'] - note['start_beat']
                    note['end_beat'] = note['start_beat'] + duration * random.uniform(0.8, 1.2)

                # Mid variations
                if 48 <= note['pitch'] < 72 and random.random() < self.intensity['mid']:
                    third = self._find_interval(note['pitch'], 3)
                    fifth = self._find_interval(note['pitch'], 5)
                    harmony_note = random.choice([third, fifth])
                    if harmony_note:
                        notes_to_add.append({'pitch': harmony_note, 'velocity': int(note['velocity'] * 0.75), 'start_beat': note['start_beat'], 'end_beat': note['end_beat']})

                # High variations
                if note['pitch'] >= 72 and random.random() < self.intensity['high']:
                    grace_note_pitch = self._find_interval(note['pitch'], 2, down=True)
                    if grace_note_pitch:
                        grace_start = max(0, note['start_beat'] - 0.125) # 32nd note before
                        notes_to_add.append({'pitch': grace_note_pitch, 'velocity': int(note['velocity'] * 0.6), 'start_beat': grace_start, 'end_beat': note['start_beat']})

            varied_loop.extend(notes_to_add)
            self.loop = varied_loop
            self.pristine_loop = copy.deepcopy(self.loop)
            self.is_quantized = False
            self._analyze_loop()

    def _find_interval(self, base_pitch, interval_steps, down=False):
        """Finds a note in the scale at a certain interval from the base_pitch."""
        try:
            base_index = self.scale_notes.index(base_pitch)
        except ValueError:
            return None

        target_index = base_index + (interval_steps - 1) * (-1 if down else 1)
        if 0 <= target_index < len(self.scale_notes):
            return self.scale_notes[target_index]
        return None

    def schedule_split(self):
        """Schedules a split to happen at the end of the current loop cycle."""
        if self.is_playing():
            self.split_is_scheduled = True
            self.logger.info("Loop split has been scheduled.")
        else:
            self.split_loop(keep_first_half=True)
            self.split_is_scheduled = False

    def set_auto_recapture_period(self, period: int):
        """Sets the period for automatic re-capture from the generator."""
        self.auto_recapture_period = period
        self.loop_cycle_count = 0
        self.logger.info(f"Auto-recapture period set to {period} loops. Cycle count reset.")

    def split_loop(self, keep_first_half: bool):
        """Splits the current loop in half and keeps either the first or second part."""
        self._flush_all_notes()
        if self.sequencer:
            self.sequencer.cancel_events_by_owner('looper')
        self._reset_playback_state()

        with self.loop_lock:
            if not self.loop:
                self.logger.warning("Cannot split loop: loop is empty.")
                return

            midpoint_beat = self.loop_duration_beats / 2.0
            if midpoint_beat <= 0:
                self.logger.warning("Cannot split loop: not enough duration.")
                return

            new_notes = []
            for note in self.loop:
                note_starts_in_first = note['start_beat'] < midpoint_beat
                note_ends_in_first = note['end_beat'] <= midpoint_beat
                note_starts_in_second = note['start_beat'] >= midpoint_beat
                note_crosses_midpoint = note_starts_in_first and note['end_beat'] > midpoint_beat

                if keep_first_half:
                    if note_starts_in_first:
                        new_note = copy.deepcopy(note)
                        new_note['end_beat'] = min(note['end_beat'], midpoint_beat)
                        new_notes.append(new_note)
                else: # Keep second half
                    if note_starts_in_second:
                        new_note = copy.deepcopy(note)
                        new_note['start_beat'] -= midpoint_beat
                        new_note['end_beat'] -= midpoint_beat
                        new_notes.append(new_note)
                    elif note_crosses_midpoint:
                        new_note = copy.deepcopy(note)
                        new_note['start_beat'] = 0
                        new_note['end_beat'] = note['end_beat'] - midpoint_beat
                        new_notes.append(new_note)

            self.loop = new_notes
            self.loop_duration_beats /= 2.0
            self.pristine_loop = copy.deepcopy(self.loop)
            self.is_quantized = False
            self._analyze_loop()

        self.logger.info(f"Loop split, kept {'first' if keep_first_half else 'second'} half. New note count: {len(self.loop or [])}")

    def unquantize_notes(self):
        """Reverts the active loop to its original, unquantized state."""
        self._flush_all_notes()
        if self.sequencer:
            self.sequencer.cancel_events_by_owner('looper')
        self._reset_playback_state()

        with self.loop_lock:
            if not self.pristine_loop:
                self.logger.warning("Cannot unquantize: no pristine loop available.")
                return

            self.loop = copy.deepcopy(self.pristine_loop)
            self.is_quantized = False
            self._analyze_loop()
        self.logger.info("Loop reverted to pristine state.")

    def quantize_notes(self, grid_resolution_beats: float):
        """Quantizes the PRISTINE loop and replaces the active loop with the result."""
        self._flush_all_notes()
        if self.sequencer:
            self.sequencer.cancel_events_by_owner('looper')
        self._reset_playback_state()

        with self.loop_lock:
            if not self.pristine_loop:
                self.logger.warning("Cannot quantize: pristine loop is empty.")
                return
            if grid_resolution_beats <= 0:
                self.logger.warning(f"Invalid grid resolution {grid_resolution_beats} for quantization.")
                return

            self.logger.info(f"Quantizing loop to {grid_resolution_beats} beat grid.")

            quantized_loop = []
            for original_note in self.pristine_loop:
                duration_beats = original_note['end_beat'] - original_note['start_beat']
                if duration_beats <= 0:
                    continue

                new_start_beat = round(original_note['start_beat'] / grid_resolution_beats) * grid_resolution_beats

                quantized_note = copy.deepcopy(original_note)
                quantized_note['start_beat'] = new_start_beat
                quantized_note['end_beat'] = new_start_beat + duration_beats
                quantized_loop.append(quantized_note)

            self.loop = quantized_loop
            self.is_quantized = True
            self._analyze_loop()

    def is_playing(self):
        """Returns True if the playback is active."""
        return self._playing.is_set()

    def play(self, start_beat: float = -1.0):
        """
        Enables playback, which is driven by the master sequencer's tick.
        Can be started at a precise future beat.
        """
        if self.is_playing():
            self.logger.warning("Playback is already active.")
            return
        if not self.loop:
            self.logger.error("Cannot play: no loop loaded.")
            return
        if not self.sequencer:
            self.logger.error("Cannot play: sequencer reference not set.")
            return

        effective_start_beat = start_beat if start_beat >= 0 else self.sequencer.current_beat
        self.loop_start_beat = effective_start_beat
        self.last_tick_beat = effective_start_beat
        self.played_notes_this_cycle.clear()
        self._is_first_tick = True
        self._playing.set()
        if self.gui_queue:
            self.gui_queue.put(('playback_started',))
        self.logger.info(f"Looper playback enabled, starting at beat {self.loop_start_beat}.")

    def stop(self):
        """Disables playback and clears any hanging notes."""
        if not self.is_playing():
            return
        self._playing.clear()
        self._flush_all_notes()
        if self.sequencer:
            self.sequencer.cancel_events_by_owner('looper')
        self._reset_playback_state()
        if self.gui_queue:
            self.gui_queue.put(('playback_stopped',))
        self.logger.info("Looper playback stopped.")

    def _flush_all_notes(self):
        """Sends note_off for all currently active notes from the looper via the sequencer."""
        self.logger.debug(f"Flushing {len(self.active_looper_notes)} active notes: {self.active_looper_notes}")
        with self.loop_lock:
            for note_pitch in list(self.active_looper_notes):
                if self.sequencer:
                    self.sequencer.play_looper_note(mido.Message('note_off', note=note_pitch, velocity=0))
            self.active_looper_notes.clear()
        self.logger.debug("Flush complete.")

    def _send_note_off(self, pitch, off_beat):
        """Schedules a note_off message using the high-precision sequencer."""
        if self.sequencer:
            self.sequencer.schedule_external_note_off(pitch, off_beat)

    def save(self, filepath: str):
        """Saves the current beat-native loop to a .mid file."""
        with self.loop_lock:
            if not self.loop:
                self.logger.error("Cannot save: no loop data.")
                raise ValueError("No loop data to save.")

            pm = pretty_midi.PrettyMIDI(initial_tempo=self.bpm)
            instrument = pretty_midi.Instrument(program=0)
            beats_to_seconds = 60.0 / self.bpm if self.bpm > 0 else 0.5

            for note_dict in self.loop:
                note = pretty_midi.Note(
                    velocity=note_dict['velocity'],
                    pitch=note_dict['pitch'],
                    start=note_dict['start_beat'] * beats_to_seconds,
                    end=note_dict['end_beat'] * beats_to_seconds
                )
                instrument.notes.append(note)

            pm.instruments.append(instrument)

            try:
                pm.write(filepath)
                self.logger.info(f"Loop saved successfully to '{filepath}'.")
            except Exception as e:
                self.logger.error(f"Error saving MIDI file: {e}")
                raise

    def load_loop(self, notes: list[dict], bpm: int):
        """
        Loads a list of notes directly into the loop, bypassing the seed.
        Used for loading presets, which store time in SECONDS.
        """
        self.logger.info(f"Loading loop with {len(notes)} notes from preset.")
        if not notes:
            with self.loop_lock:
                self.loop = None
                self.pristine_loop = None
                self.loop_duration_beats = 0.0
            return

        self.bpm = bpm
        seconds_to_beats = self.bpm / 60.0 if self.bpm > 0 else 2.0

        loaded_notes = []
        max_end_beat = 0.0
        for note_dict in notes:
            start_beat = note_dict['start'] * seconds_to_beats
            end_beat = note_dict['end'] * seconds_to_beats
            loaded_notes.append({
                'pitch': note_dict['pitch'],
                'velocity': note_dict['velocity'],
                'start_beat': start_beat,
                'end_beat': end_beat
            })
            if end_beat > max_end_beat:
                max_end_beat = end_beat

        with self.loop_lock:
            self.loop = loaded_notes
            self.pristine_loop = copy.deepcopy(loaded_notes)
            self.loop_duration_beats = max_end_beat
            self.is_quantized = False
            self._analyze_loop()

    def start_recording(self, record_length_beats: float, start_beat: float, overdub: bool = False):
        """Starts a new stream recording or an overdub session at a precise beat."""
        self.was_playing_before_record = self.is_playing()

        self.is_recording = True
        self.is_overdubbing = overdub
        if self.gui_queue:
            self.gui_queue.put(('recording_started',))

        self.recording_start_beat = start_beat
        self.recorded_notes = []

        if self.is_overdubbing:
            self.is_pending_replace = False
            if self.loop and self.loop_duration_beats > 0:
                self.record_length_beats = self.loop_duration_beats
                self.logger.info(f"Started overdubbing on existing loop of {self.record_length_beats:.2f} beats.")
            else:
                self.logger.warning("Overdub requested but no loop exists. Aborting recording.")
                self.is_recording = False
                self.is_overdubbing = False
                if self.gui_queue:
                    self.gui_queue.put(('recording_finished',))
                return
        else:
            # This is a new recording that will replace the old loop upon completion.
            # The old loop continues to play as a reference.
            self.is_pending_replace = True
            self.record_length_beats = record_length_beats
            self.logger.info(f"Started new recording (replace mode) for {record_length_beats} beats. Old loop will be replaced on stop.")

    def stop_recording(self):
        """Stops the current recording and processes the recorded notes into a beat-native loop."""
        if not self.is_recording:
            return

        self.is_recording = False
        self.logger.info(f"Stopped recording. Processing {len(self.recorded_notes)} recorded notes.")

        if self.gui_queue:
            self.gui_queue.put(('recording_finished',))

        newly_recorded_notes = []
        for note_dict in self.recorded_notes:
            start_beat_relative = note_dict['start_beat'] - self.recording_start_beat
            end_beat_relative = note_dict['end_beat'] - self.recording_start_beat
            if start_beat_relative >= self.record_length_beats:
                continue
            end_beat_relative = min(end_beat_relative, self.record_length_beats)
            start_beat_relative = max(0, start_beat_relative)
            if start_beat_relative < end_beat_relative:
                newly_recorded_notes.append({
                    'pitch': note_dict['pitch'],
                    'velocity': note_dict['velocity'],
                    'start_beat': start_beat_relative,
                    'end_beat': end_beat_relative
                })

        with self.loop_lock:
            if self.is_pending_replace:
                self.loop = newly_recorded_notes
                self.loop_duration_beats = self.record_length_beats
            elif self.is_overdubbing:
                if self.loop:
                    self.loop.extend(newly_recorded_notes)
                else:
                    self.loop = newly_recorded_notes
                    self.loop_duration_beats = self.record_length_beats

            if not self.loop:
                self.loop_duration_beats = 0.0

            self.pristine_loop = [n.copy() for n in self.loop] if self.loop else None

            self.is_quantized = False
            self.pad_mode = False
            self.is_pending_replace = False
            self._analyze_loop()

        self.is_overdubbing = False

        if self.is_pending_replace and self.loop:
            self.logger.info("New loop recorded, resetting and starting playback.")
            self._reset_playback_state()
            self.play()
        elif not self.was_playing_before_record and self.loop:
            self.logger.info("Autostarting playback of new loop.")
            self.play()

    def _reset_playback_state(self):
        """Resets all playback-related counters and states."""
        self.loop_cycle_count = 0
        self.played_notes_this_cycle.clear()
        if self.sequencer:
            self.loop_start_beat = self.sequencer.current_beat
            self.last_tick_beat = self.sequencer.current_beat
        else:
            self.loop_start_beat = 0.0
            self.last_tick_beat = 0.0
        self.logger.debug("Looper playback state has been reset.")

    def tick(self, master_beat: float):
        """The main tick method, driven by the Sequencer's clock in BEATS."""
        if self.is_recording and master_beat >= self.recording_start_beat + self.record_length_beats:
            self.logger.info("Recording duration met, stopping recording.")
            self.stop_recording()

        if not self.is_playing() or not self.loop:
            return

        with self.loop_lock:
            loop_duration = self.loop_duration_beats
            notes = sorted(self.loop, key=lambda n: n['start_beat']) if self.loop else []

        if loop_duration <= 0:
            return

        elapsed_beats = master_beat - self.loop_start_beat

        # On the first tick after play, the "previous" position was the start of the loop (0.0).
        # This establishes the first time window correctly.
        if self._is_first_tick:
            last_elapsed_beats = 0.0
            self._is_first_tick = False
        else:
            last_elapsed_beats = self.last_tick_beat - self.loop_start_beat

        current_loop_pos_beats = elapsed_beats % loop_duration
        last_loop_pos_beats = last_elapsed_beats % loop_duration

        # This condition needs to be robust against the very first tick where last and current are the same.
        if current_loop_pos_beats < last_loop_pos_beats:
            self.loop_cycle_count += 1
            self.played_notes_this_cycle.clear()
            if self.split_is_scheduled:
                self.split_loop(keep_first_half=True)
                self.split_is_scheduled = False
            if self.auto_recapture_period and self.auto_recapture_period > 0 and self.loop_cycle_count % self.auto_recapture_period == 0:
                if self.gui_queue:
                    self.gui_queue.put("recapture")

        for note in notes:
            note_start_beat = note['start_beat']
            should_play = (last_loop_pos_beats <= note_start_beat < current_loop_pos_beats) or \
                          (current_loop_pos_beats < last_loop_pos_beats and (note_start_beat >= last_loop_pos_beats or note_start_beat < current_loop_pos_beats))

            # Use a unique tuple (start_beat, pitch) to identify a note instance in a cycle
            note_id = (note_start_beat, note['pitch'])

            if note_id not in self.played_notes_this_cycle and should_play:
                if self.sequencer:
                    self.sequencer.play_looper_note(mido.Message('note_on', note=note['pitch'], velocity=note['velocity']))
                self.active_looper_notes.add(note['pitch'])

                note_duration_beats = note['end_beat'] - note['start_beat']
                note_off_beat = master_beat + note_duration_beats

                if not self.pad_mode:
                    self._send_note_off(note['pitch'], note_off_beat)

                self.played_notes_this_cycle.add(note_id)

        self.last_tick_beat = master_beat

    def add_live_note(self, note_info: dict):
        """Adds a note from the live stream to the recording buffer if active."""
        if not self.is_recording:
            return

        # Ensure end_beat is calculated correctly.
        note_info['end_beat'] = note_info['start_beat'] + note_info['duration_beats']
        self.recorded_notes.append(note_info)
