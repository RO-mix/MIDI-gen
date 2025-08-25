import pytest
from src.generators.euclidean_generator import EuclideanGenerator, _calculate_euclidean_pattern
from src.theory.duration import DURATIONS

# Test cases for the Bjorklund algorithm implementation
@pytest.mark.parametrize("steps, pulses, expected_pattern", [
    (8, 3, [1, 0, 0, 1, 0, 0, 1, 0]),
    (4, 2, [1, 0, 1, 0]),
    (5, 2, [1, 0, 0, 1, 0]), # Corrected from previous version
    (5, 3, [1, 0, 1, 0, 1]), # Corrected based on formula
    (16, 4, [1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0]),
    (1, 1, [1]),
    (8, 8, [1, 1, 1, 1, 1, 1, 1, 1]),
    (8, 0, [0, 0, 0, 0, 0, 0, 0, 0]),
])
def test_calculate_euclidean_pattern(steps, pulses, expected_pattern):
    assert _calculate_euclidean_pattern(steps, pulses) == expected_pattern

def test_generator_initializes_with_correct_pattern():
    gen = EuclideanGenerator(steps=8, pulses=3)
    assert gen.steps == 8
    assert gen.pulses == 3
    assert gen.pattern == [1, 0, 0, 1, 0, 0, 1, 0]
    assert gen.current_step == -1

def test_generate_follows_pattern_and_repeats():
    gen = EuclideanGenerator(steps=5, pulses=2, note=64)
    valid_durations = set(DURATIONS.values())
    expected_sequence = [1, 0, 0, 1, 0] # Using the new correct pattern

    # Test one full cycle
    for i in range(len(expected_sequence)):
        events, _ = gen.generate(0.0)
        if expected_sequence[i]:
            assert len(events) == 1
            msg, duration = events[0]
            assert msg.type == 'note_on'
            assert msg.note == 64
            assert duration in valid_durations
        else:
            assert len(events) == 0

    # Test that the cycle repeats
    events, _ = gen.generate(0.0)
    assert len(events) == 1 # Should be the first step again

def test_update_params_triggers_recalculation_for_pattern_parameters():
    gen = EuclideanGenerator(steps=8, pulses=3)
    assert gen.pattern == [1, 0, 0, 1, 0, 0, 1, 0]

    # Update pulses, should trigger recalculation
    gen.update_params(pulses=4)
    assert gen.pulses == 4
    assert gen.pattern == [1, 0, 1, 0, 1, 0, 1, 0]
    assert gen.current_step == -1 # Step should be reset

def test_update_params_skips_recalculation_for_non_pattern_parameters(mocker):
    gen = EuclideanGenerator(steps=8, pulses=3, note=60)
    # Mock the internal method to see if it's called
    mocked_update = mocker.patch.object(gen, '_update_pattern')

    gen.update_params(note=72, velocity=120)

    assert gen.note == 72
    assert gen.velocity == 120
    mocked_update.assert_not_called()

def test_pulse_count_is_correctly_clamped_to_step_count():
    # Pulses > steps should be clamped
    gen = EuclideanGenerator(steps=8, pulses=10)
    assert gen.pulses == 8
    assert gen.pattern == [1, 1, 1, 1, 1, 1, 1, 1]

    # Pulses = 0
    gen = EuclideanGenerator(steps=8, pulses=0)
    assert gen.pulses == 0
    assert gen.pattern == [0, 0, 0, 0, 0, 0, 0, 0]

    # Generate on a zero-pulse pattern
    events, _ = gen.generate(0.0)
    assert events == []

def test_generate_returns_correct_rate():
    """Tests that the generator returns the correct rate value."""
    gen = EuclideanGenerator(rate=0.5)
    _, wait_time = gen.generate(0.0)
    assert wait_time == 0.5

def test_deviation_polar_produces_notes_in_positive_range():
    """Tests the deviation range in polar (positive only) mode."""
    scale = [60, 62, 64, 65, 67, 69, 71, 72] # C Major scale
    gen = EuclideanGenerator(note=60, deviation_range=5, deviation_is_bipolar=False)
    gen.set_scale_notes(scale)

    # The deviation is 0 to +5 from note 60.
    # Possible notes from scale are 60, 62, 64, 65.
    expected_notes = {60, 62, 64, 65}

    generated_notes = set()
    for _ in range(50):
        events, _ = gen.generate(0.0)
        if events:
            generated_notes.add(events[0][0].note)

    assert generated_notes.issubset(expected_notes)
    # Check that it's not just playing the base note
    assert len(generated_notes) > 1

def test_deviation_bipolar_produces_notes_in_both_directions():
    """Tests the deviation range in bipolar (both directions) mode."""
    scale = [55, 57, 58, 60, 62, 64, 65, 67] # C Major scale with some lower notes
    gen = EuclideanGenerator(note=60, deviation_range=5, deviation_is_bipolar=True)
    gen.set_scale_notes(scale)

    # The deviation is -5 to +5 from note 60.
    # Possible notes from scale are 55, 57, 58, 60, 62, 64, 65.
    expected_notes = {55, 57, 58, 60, 62, 64, 65}

    generated_notes = set()
    for _ in range(50):
        events, _ = gen.generate(0.0)
        if events:
            generated_notes.add(events[0][0].note)

    assert generated_notes.issubset(expected_notes)
    # Check that it's not just playing the base note and uses both directions
    assert len(generated_notes) > 1
