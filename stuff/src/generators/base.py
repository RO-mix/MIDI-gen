import mido
from abc import ABC, abstractmethod

class BaseGenerator(ABC):
    """
    Abstract base class for all pattern generators.
    """
    def __init__(self):
        self.scale_notes = []
        self.duration_bias = 0.5  # 0.0 for long notes, 1.0 for short notes

    def set_scale_notes(self, notes: list[int]):
        """
        Sets the list of allowed notes from the current musical scale.
        This method is called by the main application.
        """
        self.scale_notes = notes

    @abstractmethod
    def generate(self, current_beat: float) -> tuple[list[tuple[mido.Message, float]], float]:
        """
        This method should be implemented by all subclasses.
        It should return a tuple containing:
        (
            list_of_midi_events: list[(mido.Message, duration_in_beats)],
            duration_to_next_event: float
        )
        The duration_in_beats determines when a note_off should be scheduled.
        The duration_to_next_event determines when the sequencer should call generate() again.
        """
        pass

    def update_params(self, **kwargs):
        """
        Updates the generator's parameters.
        Subclasses can override this to handle their specific parameters.
        """
        for key, value in kwargs.items():
            if hasattr(self, key):
                setattr(self, key, value)

    def capture(self, duration_beats: float, start_beat: float = 0.0) -> list[dict]:
        """
        Captures the generator's output for a given duration, starting from a specific beat.

        This method simulates the passage of time by repeatedly calling the
        generate() method and collecting the note_on events.

        Args:
            duration_beats: The total number of beats to capture.
            start_beat: The beat on the master timeline to start the capture from.

        Returns:
            A list of note event dictionaries with absolute beat timings.
        """
        notes = []
        current_beat = start_beat
        end_beat = start_beat + duration_beats

        # Align the starting beat to the generator's rate to get predictable captures
        if hasattr(self, 'rate') and self.rate > 0:
            current_beat = (start_beat // self.rate) * self.rate

        while current_beat < end_beat:
            generated_events, time_to_next = self.generate(current_beat)

            for msg, duration in generated_events:
                if msg.type == 'note_on' and msg.velocity > 0:
                    notes.append({
                        'pitch': msg.note,
                        'velocity': msg.velocity,
                        'start': current_beat,
                        'end': current_beat + duration
                    })

            if time_to_next <= 0:
                # Avoid infinite loops if the generator is configured incorrectly
                time_to_next = 1.0

            current_beat += time_to_next

        return notes
