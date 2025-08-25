import mido
import random
from .base import BaseGenerator
from src.theory.duration import get_probalistic_duration

def _calculate_euclidean_pattern(steps, pulses):
    """
    Calculates a Euclidean rhythm pattern using a formulaic approach.
    """
    if pulses <= 0: return [0] * steps
    if pulses > steps: pulses = steps
    pattern = []
    for i in range(steps):
        if (i * pulses) % steps < pulses:
            pattern.append(1)
        else:
            pattern.append(0)
    return pattern

class EuclideanGenerator(BaseGenerator):
    """
    A generator for creating Euclidean rhythms with static or random note selection.
    """
    def __init__(self, steps=16, pulses=4, note=60, velocity=100, channel=0, deviation_range=0, rate=0.25, deviation_is_bipolar=False, note_probability=1.0):
        super().__init__()
        self.steps = steps
        self.pulses = pulses
        self.note = note
        self.velocity = velocity
        self.channel = channel
        self.deviation_range = deviation_range
        self.rate = rate
        self.deviation_is_bipolar = deviation_is_bipolar
        self.note_probability = note_probability

        self.current_step = -1
        self.pattern = []
        self._update_pattern()

    def _update_pattern(self):
        """Recalculates the Euclidean pattern."""
        self.steps = max(1, self.steps)
        self.pulses = max(0, min(self.pulses, self.steps))
        self.pattern = _calculate_euclidean_pattern(self.steps, self.pulses)
        self.current_step = -1

    def generate(self, current_beat: float) -> tuple[list[tuple[mido.Message, float]], float]:
        """
        Generates the next event in the Euclidean sequence.
        Returns the events and a fixed duration of 1.0 beat until the next step.
        """
        events = []
        if not self.pattern:
            return [], 1.0

        self.current_step = (self.current_step + 1) % self.steps

        if self.pattern[self.current_step] and random.random() < self.note_probability:
            note_val = self.note
            if self.deviation_range > 0 and self.scale_notes:
                base_note = int(round(self.note))
                if self.deviation_is_bipolar:
                    min_dev_note = base_note - self.deviation_range
                else:
                    min_dev_note = base_note
                max_dev_note = base_note + self.deviation_range
                possible_notes = [n for n in self.scale_notes if min_dev_note <= n <= max_dev_note]
                if possible_notes:
                    note_val = random.choice(possible_notes)
                # If no notes in range, fallback to the base self.note

            note_on_msg = mido.Message('note_on', note=note_val, velocity=self.velocity, channel=self.channel)
            duration = get_probalistic_duration(self.duration_bias)
            events.append((note_on_msg, duration))

        return events, self.rate

    def update_params(self, **kwargs):
        """
        Updates generator's parameters.
        """
        pattern_changed = False
        for key, value in kwargs.items():
            if hasattr(self, key) and getattr(self, key) != value:
                setattr(self, key, value)
                if key in ['steps', 'pulses']:
                    pattern_changed = True

        if pattern_changed:
            self._update_pattern()
