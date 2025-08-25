import pytest
from src.theory.scales import get_notes_in_scale, SCALES

def test_get_notes_in_c_major_scale():
    # Test C Major scale across a few octaves
    c_major_intervals = SCALES["Major"]
    expected_notes = [60, 62, 64, 65, 67, 69, 71, 72, 74, 76]
    notes = get_notes_in_scale(root_note=0, scale_intervals=c_major_intervals, min_note=60, max_note=76)
    assert notes == expected_notes

def test_get_notes_in_a_minor_pentatonic():
    # Test A Minor Pentatonic
    a_minor_penta_intervals = SCALES["Minor Pentatonic"]
    # Root A is 9. A4 is 57+12=69. A5 is 81.
    expected_notes = [69, 72, 74, 76, 79, 81]
    notes = get_notes_in_scale(root_note=9, scale_intervals=a_minor_penta_intervals, min_note=69, max_note=81)
    assert notes == expected_notes

def test_get_notes_in_scale_respects_min_and_max_note_range():
    # Test that min_note and max_note are respected
    c_major_intervals = SCALES["Major"]
    notes = get_notes_in_scale(root_note=0, scale_intervals=c_major_intervals, min_note=63, max_note=68)
    assert notes == [64, 65, 67]

def test_get_notes_in_scale_with_empty_intervals_returns_empty_list():
    # Test with empty intervals list
    notes = get_notes_in_scale(root_note=0, scale_intervals=[], min_note=60, max_note=72)
    assert notes == []

def test_get_notes_in_scale_returns_empty_list_when_no_scale_notes_are_in_range():
    # Test a valid scale but a range where no notes fall
    c_major_intervals = SCALES["Major"]
    notes = get_notes_in_scale(root_note=0, scale_intervals=c_major_intervals, min_note=61, max_note=61)
    assert notes == []

@pytest.mark.parametrize("scale_name, expected_notes_from_c4", [
    ("Hirajoshi",         [60, 62, 63, 67, 68, 72]),
    ("Iwato",             [60, 61, 65, 66, 70, 72]),
    ("Hungarian Minor",   [60, 62, 63, 66, 67, 68, 71, 72]),
    ("Phrygian Dominant", [60, 61, 64, 65, 67, 68, 70, 72]),
    ("Enigmatic",         [60, 61, 64, 66, 68, 70, 71, 72]),
])
def test_new_exotic_scales(scale_name, expected_notes_from_c4):
    """Tests the newly added exotic scales."""
    intervals = SCALES[scale_name]
    notes = get_notes_in_scale(root_note=0, scale_intervals=intervals, min_note=60, max_note=72)
    assert notes == expected_notes_from_c4
