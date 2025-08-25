import mido
import random
from .base import BaseGenerator
from ..theory.scales import get_notes_in_scale, SCALES

class RandomGeneratorV2(BaseGenerator):
    """
    An "ambient" random generator that alternates between single long notes
    and rapid "bursts" of notes, with advanced filtering for bass notes.
    """
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.name = "Случайный v2.2"
        self.slug = "random_v2"

        # Note selection
        self.min_note = 48
        self.max_note = 72
        self.scale_name = "Major"
        self.root_note = 60

        # --- Ambient Burst Engine ---
        # Duration of a full cycle (e.g., 4.0 for a whole note)
        self.base_duration = 4.0
        # Probability (0-1) of a burst occurring in a cycle
        self.burst_probability = 0.5
        # 8-step pattern of probabilities for notes within a burst
        self.burst_pattern = [1.0, 0.5, 0.8, 0.0, 0.9, 0.4, 0.0, 0.6]
        # Speed of notes within the burst (e.g., 1/4 for 16ths)
        self.acceleration_strength = 1/4
        # Global probability for any note to be generated
        self.note_probability = 1.0

        # --- State Variables ---
        self.velocity = 100
        self.channel = 0
        self.is_in_burst = False
        self.burst_step = 0
        self.scale_notes = []
        self.bass_notes = []

        # Initial setup
        self.set_scale(self.root_note, self.scale_name)

    def set_scale(self, root_note, scale_name):
        self.root_note = root_note
        self.scale_name = scale_name

        if scale_name in SCALES:
            scale_intervals = SCALES[scale_name]
            # Get all notes in the scale across all octaves first
            full_scale = get_notes_in_scale(self.root_note % 12, scale_intervals)
            # Then filter them by the generator's min/max note range
            self.scale_notes = [n for n in full_scale if self.min_note <= n <= self.max_note]
        else:
            self.scale_notes = []

        # Identify bass notes (root and fifth) in the lowest octave
        root = self.root_note % 12
        fifth = (self.root_note + 7) % 12
        self.bass_notes = [
            n for n in self.scale_notes
            if n < self.min_note + 12 and (n % 12 == root or n % 12 == fifth)
        ]

    def _get_random_note(self):
        """
        Gets a random note, applying bass filtering if applicable.
        """
        # If there are notes in the lowest octave, give a 50% chance to play a bass note
        lowest_octave_notes = [n for n in self.scale_notes if n < self.min_note + 12]
        if lowest_octave_notes and self.bass_notes and random.random() < 0.5:
            return random.choice(self.bass_notes)

        # Otherwise, or if the roll fails, play any note from the allowed scale
        if not self.scale_notes:
            return None
        return random.choice(self.scale_notes)

    def generate(self, current_beat):
        events = []

        if not self.is_in_burst:
            # --- New Cycle Decision ---
            self.is_in_burst = random.random() < self.burst_probability
            self.burst_step = 0

            if not self.is_in_burst:
                # --- Play a Single Long Note ---
                if random.random() < self.note_probability:
                    note_val = self._get_random_note()
                    if note_val is not None:
                        msg = mido.Message('note_on', note=note_val, velocity=self.velocity, channel=self.channel)
                        events.append((msg, self.base_duration * 0.9))
                return events, self.base_duration

        # --- Process Burst ---
        if self.is_in_burst:
            # Check if the burst pattern is over
            if self.burst_step >= len(self.burst_pattern):
                self.is_in_burst = False
                time_consumed_by_burst = len(self.burst_pattern) * self.acceleration_strength
                wait_time = self.base_duration - time_consumed_by_burst
                return [], max(0.01, wait_time) # Wait for the rest of the cycle

            # --- Process a single step in the burst pattern ---
            step_probability = self.burst_pattern[self.burst_step]
            # Combine the pattern's probability with the global note probability
            if random.random() < step_probability and random.random() < self.note_probability:
                note_val = self._get_random_note()
                if note_val is not None:
                    note_duration = self.acceleration_strength * 0.95
                    msg = mido.Message('note_on', note=note_val, velocity=self.velocity, channel=self.channel)
                    events.append((msg, note_duration))

            self.burst_step += 1
            return events, self.acceleration_strength

        # Fallback (should not be reached)
        return [], self.base_duration

    def get_ui_layout(self):
        # UI layout will be defined in a subsequent step.
        return []
