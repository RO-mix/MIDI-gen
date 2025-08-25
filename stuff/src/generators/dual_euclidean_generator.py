import random
import mido
from .base import BaseGenerator
from .euclidean_generator import _calculate_euclidean_pattern
from src.theory.duration import get_probalistic_duration

class DualEuclideanGenerator(BaseGenerator):
    """
    A generator that combines two independent Euclidean rhythm machines
    to create polyrhythms. Each machine can have its own note generation mode.
    """
    def __init__(self, channel=0, **kwargs):
        super().__init__()
        self.channel = channel

        # Machine A
        self.steps_a = 16
        self.pulses_a = 4
        self.note_a = 60
        self.velocity_a = 100
        self.duration_bias_a = 0.5
        self.deviation_range_a = 0
        self.deviation_is_bipolar_a = False

        # Machine B
        self.steps_b = 15
        self.pulses_b = 4
        self.note_b = 67
        self.velocity_b = 100
        self.duration_bias_b = 0.5
        self.deviation_range_b = 0
        self.deviation_is_bipolar_b = False

        # Global
        self.rate = 0.25
        self.note_probability = 1.0

        self.pattern_a = []
        self.pattern_b = []
        self.master_step = -1

        # Set initial params and update patterns
        self.update_params(**kwargs)
        self._update_patterns()

    def _update_patterns(self):
        """Recalculates the patterns for both machines."""
        self.pattern_a = _calculate_euclidean_pattern(self.steps_a, self.pulses_a)
        self.pattern_b = _calculate_euclidean_pattern(self.steps_b, self.pulses_b)
        self.master_step = -1

    def get_notes_in_range(self) -> list[int]:
        """
        Returns the full list of scale notes. Unlike RandomGenerator, this generator
        doesn't have its own min/max note range, so it uses the full scale.
        """
        return self.scale_notes or list(range(128))

    def generate(self, current_beat: float) -> tuple[list[tuple[mido.Message, float]], float]:
        """
        Generates the next step for both machines and returns MIDI events.
        Returns the events and a fixed duration of 1.0 beat until the next step.
        """
        self.master_step += 1
        events = []

        # Check the global probability for generating any notes on this step
        if random.random() > self.note_probability:
            return events, self.rate

        # --- Machine A ---
        if self.pattern_a:
            current_step_a = self.master_step % len(self.pattern_a)
            if self.pattern_a[current_step_a]:
                note_val_a = self.note_a
                available_notes = self.get_notes_in_range()
                if self.deviation_range_a > 0 and available_notes:
                    if self.deviation_is_bipolar_a:
                        min_dev_note = self.note_a - self.deviation_range_a
                    else:
                        min_dev_note = self.note_a
                    max_dev_note = self.note_a + self.deviation_range_a
                    possible_notes = [n for n in available_notes if min_dev_note <= n <= max_dev_note]
                    if possible_notes:
                        note_val_a = random.choice(possible_notes)

                msg = mido.Message('note_on', note=note_val_a, velocity=self.velocity_a, channel=self.channel)
                duration = get_probalistic_duration(self.duration_bias_a)
                events.append((msg, duration))

        # --- Machine B ---
        if self.pattern_b:
            current_step_b = self.master_step % len(self.pattern_b)
            if self.pattern_b[current_step_b]:
                note_val_b = self.note_b
                available_notes = self.get_notes_in_range()
                if self.deviation_range_b > 0 and available_notes:
                    if self.deviation_is_bipolar_b:
                        min_dev_note = self.note_b - self.deviation_range_b
                    else:
                        min_dev_note = self.note_b
                    max_dev_note = self.note_b + self.deviation_range_b
                    possible_notes = [n for n in available_notes if min_dev_note <= n <= max_dev_note]
                    if possible_notes:
                        note_val_b = random.choice(possible_notes)

                msg = mido.Message('note_on', note=note_val_b, velocity=self.velocity_b, channel=self.channel)
                duration = get_probalistic_duration(self.duration_bias_b)
                events.append((msg, duration))

        return events, self.rate

    def update_params(self, **kwargs):
        """
        Updates parameters and recalculates patterns if necessary.
        """
        patterns_changed = False
        for key, value in kwargs.items():
            if hasattr(self, key) and getattr(self, key) != value:
                setattr(self, key, value)
                if key in ['steps_a', 'pulses_a', 'steps_b', 'pulses_b']:
                    patterns_changed = True

        if patterns_changed:
            self._update_patterns()
