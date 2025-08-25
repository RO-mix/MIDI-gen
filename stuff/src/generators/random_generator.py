import random
import mido
import math
from .base import BaseGenerator
from src.theory.duration import get_probalistic_duration

class RandomGenerator(BaseGenerator):
    """
    A generator that produces random MIDI notes, constrained by an optional musical scale.
    """
    def __init__(self, min_note=60, max_note=72, max_velocity=127, velocity_bias=0.5, add_cc74=False, channel=0, note_probability=1.0, rate=1.0):
        super().__init__()
        self.min_note = min_note
        self.max_note = max_note
        self.max_velocity = max_velocity
        self.velocity_bias = velocity_bias
        self.add_cc74 = add_cc74
        self.channel = channel
        self.note_probability = note_probability
        self.rate = rate

    def get_notes_in_range(self) -> list[int]:
        """
        Filters the full list of scale notes by the generator's min/max note range.
        If no scale is active, it returns all notes within the range.
        """
        if not self.scale_notes:
            # If no scale is active, return the full range of notes
            return list(range(self.min_note, self.max_note + 1))

        # Filter the provided scale notes by our min/max range
        return [note for note in self.scale_notes if self.min_note <= note <= self.max_note]

    def generate(self, current_beat: float) -> tuple[list[tuple[mido.Message, float]], float]:
        """
        Generates a random MIDI note-on message, respecting the current scale.
        Returns the events and a fixed duration of 1.0 beat until the next event.
        """
        # Decide whether to generate a note based on probability
        if random.random() > self.note_probability:
            return [], self.rate # No note, just wait

        events = []

        available_notes = self.get_notes_in_range()
        if not available_notes:
            return [], self.rate # No notes available, wait

        random_note = random.choice(available_notes)

        # Velocity calculation using beta distribution for bias
        bias = 1.0 - self.velocity_bias
        if bias < 0.5:
            alpha = 1 + (0.5 - bias) * 8
            beta = 1
        else:
            alpha = 1
            beta = 1 + (bias - 0.5) * 8

        max_v = max(1, self.max_velocity)
        random_val = random.betavariate(alpha, beta)
        random_velocity = int(1 + random_val * (max_v - 1))
        random_velocity = max(1, random_velocity)

        note_on_msg = mido.Message('note_on',
                                   note=random_note,
                                   velocity=random_velocity,
                                   channel=self.channel)

        duration = get_probalistic_duration(self.duration_bias)
        events.append((note_on_msg, duration))

        if self.add_cc74:
            random_cc_value = random.randint(0, 127)
            cc_msg = mido.Message('control_change',
                                  control=74,
                                  value=random_cc_value,
                                  channel=self.channel)
            events.append((cc_msg, 0))

        return events, self.rate
