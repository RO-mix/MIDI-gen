import pytest
from unittest.mock import patch
from src.looper.looper_module import LooperModule

# The `looper_test_data` fixture is now available from conftest.py

def test_find_interval_finds_correct_notes(looper_test_data):
    """
    Tests the internal _find_interval method for correctly identifying scale degrees.
    """
    looper = looper_test_data['looper']

    # Setup a C Major scale
    looper.scale_notes = [60, 62, 64, 65, 67, 69, 71, 72] # C4, D4, E4, F4, G4, A4, B4, C5

    # Test finding a third up from E (should be G)
    assert looper._find_interval(64, 3) == 67

    # Test finding a fifth up from C (should be G)
    assert looper._find_interval(60, 5) == 67

    # Test finding a second down from D (should be C)
    assert looper._find_interval(62, 2, down=True) == 60

    # Test edge case: interval goes out of bounds
    assert looper._find_interval(72, 3) is None # A third up from C5 is not in the list

    # Test edge case: base_pitch not in scale
    assert looper._find_interval(61, 3) is None

def test_generate_variations_with_bass_intensity(looper_test_data):
    """
    Tests that generate_variations adds lower octave notes when bass intensity is high.
    """
    looper = looper_test_data['looper']

    # Setup a scale that includes C2 and C1
    looper.scale_notes = [24, 36] # C1, C2

    # Capture a simple loop with a bass note (pitch < 48)
    notes = [{'pitch': 36, 'velocity': 100, 'start': 0.0, 'end': 1.0}] # C2
    looper.capture_seed(notes, looper.scale_notes, 24, [], 120, 4.0)

    original_note_count = len(looper.loop)
    assert original_note_count == 1

    # Set high bass intensity and mock random to ensure the variation happens
    looper.intensity['bass'] = 1.0
    with patch('random.random', return_value=0.0):
        looper.generate_variations()

    # Assert that a new note has been added
    assert len(looper.loop) > original_note_count

    # Assert that a lower note was added (C2 - 12 = C1 which is 24)
    pitches = [n['pitch'] for n in looper.loop]
    assert 24 in pitches
