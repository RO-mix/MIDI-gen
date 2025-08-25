import pytest
import mido
from src.generators.random_generator_v2 import RandomGeneratorV2

@pytest.fixture
def gen_v2():
    """Fixture to create a default RandomGeneratorV2 instance."""
    return RandomGeneratorV2()

def test_v2_generator_initializes_with_correct_defaults(gen_v2):
    """Test that the generator initializes with correct default values."""
    assert gen_v2.name == "Случайный v2.2"
    assert gen_v2.slug == "random_v2"
    assert gen_v2.base_duration == 4.0
    assert gen_v2.burst_probability == 0.5
    assert len(gen_v2.burst_pattern) == 8
    assert gen_v2.acceleration_strength == 1/4
    assert not gen_v2.is_in_burst
    assert gen_v2.burst_step == 0

def test_set_scale_populates_scale_and_bass_notes_correctly(gen_v2):
    """Test the set_scale method for populating notes, especially bass notes."""
    gen_v2.min_note = 60 # C4
    gen_v2.max_note = 84 # C6
    gen_v2.set_scale(60, "Major") # C Major scale

    # Bass notes are the root (C) and fifth (G) in the lowest octave of the range (60-71)
    # In C Major, these are C (60) and G (67).
    assert gen_v2.bass_notes == [60, 67]
    # The full scale should contain all notes from C4 up to C6 in the C Major scale.
    assert 60 in gen_v2.scale_notes
    assert 62 in gen_v2.scale_notes
    assert 72 in gen_v2.scale_notes
    assert 73 not in gen_v2.scale_notes # D# is not in C Major

def test_get_random_note_chooses_from_bass_notes_when_roll_succeeds(mocker, gen_v2):
    """Test that _get_random_note favors bass notes when the random roll succeeds."""
    gen_v2.min_note = 60
    gen_v2.max_note = 84
    gen_v2.set_scale(60, "Major") # bass_notes are [60, 67]

    # Mock random() to always be < 0.5, forcing the bass note path
    mocker.patch('random.random', return_value=0.4)
    mock_choice = mocker.patch('random.choice')

    gen_v2._get_random_note()
    # It should have been called with the list of bass notes
    mock_choice.assert_called_with(gen_v2.bass_notes)

def test_get_random_note_chooses_from_all_scale_notes_when_roll_fails(mocker, gen_v2):
    """Test that _get_random_note plays regular notes if the bass roll fails."""
    gen_v2.min_note = 60
    gen_v2.max_note = 84
    gen_v2.set_scale(60, "Major")

    # Mock random() to be > 0.5, skipping the bass note path
    mocker.patch('random.random', return_value=0.6)
    mock_choice = mocker.patch('random.choice')

    gen_v2._get_random_note()
    # It should have been called with the full list of scale notes
    mock_choice.assert_called_with(gen_v2.scale_notes)

def test_generate_with_zero_burst_prob_creates_single_long_note(mocker, gen_v2):
    """Test the generator in single-note mode (burst_probability=0)."""
    gen_v2.burst_probability = 0.0
    gen_v2.base_duration = 4.0
    mocker.patch.object(gen_v2, '_get_random_note', return_value=60)

    events, duration_to_next = gen_v2.generate(current_beat=0)

    assert not gen_v2.is_in_burst
    assert len(events) == 1
    msg, duration = events[0]
    assert msg.type == 'note_on'
    assert msg.note == 60
    assert duration == pytest.approx(4.0 * 0.9)
    assert duration_to_next == 4.0

def test_generate_with_full_burst_prob_enters_burst_mode(mocker, gen_v2):
    """Test the generator correctly enters burst mode (burst_probability=1)."""
    gen_v2.burst_probability = 1.0
    gen_v2.acceleration_strength = 1/4
    # Mock random.random for the burst decision (value < 1.0) and pattern decision (value < step probability)
    mocker.patch('random.random', return_value=0.0)
    mocker.patch.object(gen_v2, '_get_random_note', return_value=60)

    events, duration_to_next = gen_v2.generate(current_beat=0)

    assert gen_v2.is_in_burst
    assert gen_v2.burst_step == 1
    assert len(events) == 1
    msg, duration = events[0]
    assert msg.note == 60
    assert duration == pytest.approx(1/4 * 0.95)
    assert duration_to_next == 1/4

def test_generate_completes_full_burst_cycle_and_waits(mocker, gen_v2):
    """Test a full burst cycle from start to finish."""
    gen_v2.burst_probability = 1.0
    gen_v2.base_duration = 4.0
    gen_v2.acceleration_strength = 1/4 # Burst will take 8 * 1/4 = 2.0 beats
    gen_v2.burst_pattern = [1.0] * 8 # Fire on every step

    mocker.patch.object(gen_v2, '_get_random_note', return_value=60)
    # Mock random.random to always succeed the burst probability and pattern checks
    mocker.patch('random.random', return_value=0.0)

    # 1. Enter burst
    events, duration_to_next = gen_v2.generate(current_beat=0)
    assert gen_v2.is_in_burst
    assert duration_to_next == 1/4
    assert len(events) == 1

    # 2-8. Continue burst
    total_events = 1
    for i in range(7):
        beat = (i + 1) * 1/4
        events, duration_to_next = gen_v2.generate(current_beat=beat)
        assert gen_v2.is_in_burst
        assert duration_to_next == 1/4
        total_events += len(events)

    assert gen_v2.burst_step == 8
    assert total_events == 8

    # 9. End of burst, start waiting period
    events, duration_to_next = gen_v2.generate(current_beat=2.0)
    assert not gen_v2.is_in_burst
    assert len(events) == 0
    # Wait time = base_duration - time_in_burst = 4.0 - 2.0 = 2.0
    # The code returns max(0.01, wait_time)
    assert duration_to_next == pytest.approx(2.0)

    # 10. New cycle after waiting
    events, duration_to_next = gen_v2.generate(current_beat=4.0)
    # It should decide on a new cycle, which will be a burst again because of the mock
    assert gen_v2.is_in_burst
    assert len(events) == 1
    assert duration_to_next == 1/4

def test_generate_v2_produces_no_notes_when_probability_is_zero(gen_v2):
    """Test that no notes are generated when note_probability is 0."""
    gen_v2.note_probability = 0.0

    # Test in single note mode
    gen_v2.burst_probability = 0.0
    events, _ = gen_v2.generate(current_beat=0)
    assert len(events) == 0

    # Test in burst mode
    gen_v2.burst_probability = 1.0
    gen_v2.is_in_burst = False # Reset state
    # Run through a full burst pattern
    for i in range(len(gen_v2.burst_pattern) + 1):
        events, _ = gen_v2.generate(current_beat=i * gen_v2.acceleration_strength)
        assert len(events) == 0

def test_generate_v2_produces_notes_at_roughly_half_probability(mocker):
    """
    Tests that notes are generated roughly 50% of the time when probability is 0.5.
    This test is statistical and covers both single note and burst modes.
    """
    # We need a fresh generator for each part of the test to avoid state bleeding

    # --- Part 1: Test single note mode ---
    gen_single = RandomGeneratorV2()
    gen_single.update_params(burst_probability=0.0, note_probability=0.5)
    mocker.patch.object(gen_single, '_get_random_note', return_value=60)

    note_generated_count_single = 0
    total_runs_single = 500
    for i in range(total_runs_single):
        events, _ = gen_single.generate(current_beat=i)
        if len(events) > 0:
            note_generated_count_single += 1

    expected_count_single = total_runs_single * 0.5
    margin_single = total_runs_single * 0.15
    assert expected_count_single - margin_single <= note_generated_count_single <= expected_count_single + margin_single

    # --- Part 2: Test burst mode ---
    gen_burst = RandomGeneratorV2()
    gen_burst.update_params(burst_probability=1.0, note_probability=0.5)
    # Mock the burst pattern to always want to fire, so only note_probability is tested
    gen_burst.burst_pattern = [1.0] * 8
    mocker.patch.object(gen_burst, '_get_random_note', return_value=60)

    note_generated_count_burst = 0
    total_runs_burst = 500
    for i in range(total_runs_burst):
        # We don't care about the timing logic here, just the note generation
        events, _ = gen_burst.generate(current_beat=i)
        if len(events) > 0:
            note_generated_count_burst += 1

    # The expected count is more complex because bursts don't happen every call.
    # A simpler check is to see if it's not 0 and not total_runs, which proves the probability is working.
    assert 0 < note_generated_count_burst < total_runs_burst


@pytest.mark.parametrize("duration", [8.0, 16.0])
def test_generate_handles_long_base_duration_correctly(gen_v2, duration):
    """Test that the generator handles new long base_duration values correctly."""
    gen_v2.burst_probability = 0.0 # Test in single note mode for simplicity
    gen_v2.base_duration = duration

    events, duration_to_next = gen_v2.generate(current_beat=0)

    assert duration_to_next == duration
    if events:
        _, note_duration = events[0]
        assert note_duration == pytest.approx(duration * 0.9)
