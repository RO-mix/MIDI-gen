import pytest
from src.generators.dual_euclidean_generator import DualEuclideanGenerator

def test_generator_initializes_with_default_values():
    gen = DualEuclideanGenerator()
    assert gen.steps_a == 16
    assert gen.pulses_a == 4
    assert gen.steps_b == 15
    assert gen.pulses_b == 4
    assert len(gen.pattern_a) == 16
    assert len(gen.pattern_b) == 15

def test_generates_correct_polyrhythm_for_3_against_4():
    # Test a 3 against 4 rhythm
    gen = DualEuclideanGenerator(steps_a=4, pulses_a=4, note_a=60, steps_b=3, pulses_b=3, note_b=67)
    # This should produce a note from each machine on every step
    for i in range(12):
        events, _ = gen.generate(0.0)
        assert len(events) == 2
        notes = {e[0].note for e in events}
        assert notes == {60, 67}

def test_update_params_triggers_recalculation_for_steps_and_pulses(mocker):
    gen = DualEuclideanGenerator()
    mocked_update = mocker.patch.object(gen, '_update_patterns')

    gen.update_params(steps_a=8, pulses_b=2)

    mocked_update.assert_called_once()

def test_update_params_skips_recalculation_for_note_and_deviation(mocker):
    gen = DualEuclideanGenerator()
    mocked_update = mocker.patch.object(gen, '_update_patterns')

    gen.update_params(note_a=70, deviation_range_a=12)

    mocked_update.assert_not_called()

def test_generate_returns_correctly_configured_rate():
    """Tests that the generator returns the correct global rate value."""
    gen = DualEuclideanGenerator(rate=0.125)
    _, wait_time = gen.generate(0.0)
    assert wait_time == 0.125

def test_deviation_for_machine_a_produces_notes_in_correct_range():
    """Tests the deviation settings for machine A."""
    scale = [60, 62, 64, 65, 67, 69, 71, 72] # C Major
    gen = DualEuclideanGenerator(
        steps_a=1, pulses_a=1, note_a=64, deviation_range_a=4, deviation_is_bipolar_a=True, # Bipolar, range 60-68
        steps_b=1, pulses_b=0 # Turn off machine B
    )
    gen.set_scale_notes(scale)

    expected_notes = {60, 62, 64, 65, 67}
    generated_notes = set()
    for _ in range(50):
        events, _ = gen.generate(0.0)
        if events:
            generated_notes.add(events[0][0].note)

    assert generated_notes.issubset(expected_notes)
    assert len(generated_notes) > 1

def test_generate_produces_no_notes_when_probability_is_zero():
    """Tests that no notes are generated when probability is 0."""
    gen = DualEuclideanGenerator(note_probability=0.0)
    # Ensure patterns would otherwise generate notes
    gen.update_params(steps_a=1, pulses_a=1, steps_b=1, pulses_b=1)

    for _ in range(50):
        events, _ = gen.generate(0.0)
        assert len(events) == 0

def test_generate_produces_notes_at_roughly_half_probability():
    """Tests that notes are generated roughly 50% of the time when probability is 0.5."""
    gen = DualEuclideanGenerator(note_probability=0.5)
    gen.update_params(steps_a=1, pulses_a=1, steps_b=1, pulses_b=1)

    note_generated_count = 0
    total_runs = 500
    for _ in range(total_runs):
        events, _ = gen.generate(0.0)
        if len(events) > 0:
            note_generated_count += 1

    expected_count = total_runs * 0.5
    margin = total_runs * 0.15
    assert expected_count - margin <= note_generated_count <= expected_count + margin

def test_deviation_for_machine_b_produces_notes_in_correct_range():
    """Tests the deviation settings for machine B."""
    scale = [60, 62, 64, 65, 67, 69, 71, 72] # C Major
    gen = DualEuclideanGenerator(
        steps_a=1, pulses_a=0, # Turn off machine A
        steps_b=1, pulses_b=1, note_b=69, deviation_range_b=3, deviation_is_bipolar_b=False # Polar, range 69-72
    )
    gen.set_scale_notes(scale)

    expected_notes = {69, 71, 72}
    generated_notes = set()
    for _ in range(50):
        events, _ = gen.generate(0.0)
        if events:
            generated_notes.add(events[0][0].note)

    assert generated_notes.issubset(expected_notes)
    assert len(generated_notes) > 1
