# Music theory constants and helpers

NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]

SCALES = {
    "Major": [0, 2, 4, 5, 7, 9, 11],
    "Minor": [0, 2, 3, 5, 7, 8, 10],
    "Dorian": [0, 2, 3, 5, 7, 9, 10],
    "Phrygian": [0, 1, 3, 5, 7, 8, 10],
    "Lydian": [0, 2, 4, 6, 7, 9, 11],
    "Mixolydian": [0, 2, 4, 5, 7, 9, 10],
    "Locrian": [0, 1, 3, 4, 7, 8, 10],
    "Harmonic Minor": [0, 2, 3, 5, 7, 8, 11],
    "Melodic Minor": [0, 2, 3, 5, 7, 9, 11],
    "Major Pentatonic": [0, 2, 4, 7, 9],
    "Minor Pentatonic": [0, 3, 5, 7, 10],
    "Blues": [0, 3, 5, 6, 7, 10],
    "Whole Tone": [0, 2, 4, 6, 8, 10],
    "Chromatic": list(range(12)),
    "Hirajoshi": [0, 2, 3, 7, 8],
    "Iwato": [0, 1, 5, 6, 10],
    "Hungarian Minor": [0, 2, 3, 6, 7, 8, 11],
    "Phrygian Dominant": [0, 1, 4, 5, 7, 8, 10],
    "Enigmatic": [0, 1, 4, 6, 8, 10, 11],
}

def get_notes_in_scale(root_note: int, scale_intervals: list[int], min_note: int = 0, max_note: int = 127) -> list[int]:
    """
    Generates a list of all MIDI notes for a given scale within a specified range.

    Args:
        root_note: The root note of the scale (0-11).
        scale_intervals: A list of intervals from the root note (e.g., [0, 2, 4, ...]).
        min_note: The minimum MIDI note number for the output.
        max_note: The maximum MIDI note number for the output.

    Returns:
        A sorted list of MIDI note numbers that fall within the scale and range.
    """
    if not scale_intervals:
        return []

    notes = set()
    # Generate notes for all octaves
    for octave in range(-1, 11):  # From C-1 to C10
        for interval in scale_intervals:
            note = root_note + interval + (octave * 12)
            if min_note <= note <= max_note:
                notes.add(note)

    return sorted(list(notes))
