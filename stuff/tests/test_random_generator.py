import pytest
import mido
from src.generators.random_generator import RandomGenerator
from src.theory.duration import DURATIONS

def test_generate_returns_correctly_structured_event_list():
    """
    Tests that the generate method returns a list of (message, duration) tuples.
    """
    gen = RandomGenerator()
    events, dur = gen.generate(0.0)
    assert isinstance(events, list)
    assert isinstance(dur, float)
    assert len(events) > 0
    for event in events:
        assert isinstance(event, tuple)
        assert len(event) == 2
        assert isinstance(event[0], mido.Message)
        assert isinstance(event[1], (float, int))

def test_generate_creates_note_on_message_with_correct_properties():
    """
    Tests the content of the generated note_on message.
    """
    gen = RandomGenerator(min_note=60, max_note=72, max_velocity=110, channel=1)
    events, _ = gen.generate(0.0)
    note_on_msg, duration = events[0]

    assert note_on_msg.type == 'note_on'
    assert 60 <= note_on_msg.note <= 72
    assert 1 <= note_on_msg.velocity <= 110
    assert note_on_msg.channel == 1

def test_generate_selects_duration_from_valid_durations():
    """
    Tests that the generated duration is one of the valid musical durations.
    """
    gen = RandomGenerator()
    valid_durations = set(DURATIONS.values())
    for _ in range(20): # Run a few times to be sure
        events, _ = gen.generate(0.0)
        _, duration = events[0]
        assert duration in valid_durations


def test_generate_adds_cc74_message_when_add_cc74_is_true():
    """
    Tests that a CC74 message is added when add_cc74 is True.
    """
    gen = RandomGenerator(add_cc74=True, channel=2)
    events, _ = gen.generate(0.0)
    assert len(events) == 2

    # Find the CC message
    cc_msg, duration = next((e for e in events if e[0].type == 'control_change'), (None, None))

    assert cc_msg is not None
    assert cc_msg.type == 'control_change'
    assert cc_msg.control == 74
    assert 0 <= cc_msg.value <= 127
    assert cc_msg.channel == 2
    assert duration == 0 # CC messages should have immediate effect

def test_generate_omits_cc74_message_when_add_cc74_is_false():
    """
    Tests that no CC74 message is added when add_cc74 is False.
    """
    gen = RandomGenerator(add_cc74=False)
    events, _ = gen.generate(0.0)
    assert len(events) == 1
    assert events[0][0].type == 'note_on'

def test_generate_only_produces_notes_from_the_provided_scale():
    """
    Tests that the generator only produces notes from the provided scale list.
    """
    gen = RandomGenerator(min_note=0, max_note=127)
    scale_notes = [60, 64, 67] # C Major triad
    gen.set_scale_notes(scale_notes)

    for _ in range(20): # Generate a bunch of notes
        events, _ = gen.generate(0.0)
        note_on_msg, _ = events[0]
        assert note_on_msg.note in scale_notes

def test_generate_produces_no_events_if_scale_and_range_are_mutually_exclusive():
    """
    Tests that the generator produces no events if the scale and range leave no notes.
    """
    gen = RandomGenerator(min_note=70, max_note=80)
    scale_notes = [60, 64, 67] # C Major triad, all notes outside the generator's range
    gen.set_scale_notes(scale_notes)

    events, _ = gen.generate(0.0)
    assert len(events) == 0

def test_generate_returns_correct_rate():
    """
    Tests that the generator returns the correct rate value for waiting.
    """
    gen = RandomGenerator(rate=2.5)
    _, wait_time = gen.generate(0.0)
    assert wait_time == 2.5

def test_generate_produces_no_events_when_probability_is_zero():
    """
    Tests that no notes are generated when note_probability is 0.
    """
    gen = RandomGenerator(note_probability=0.0)
    for _ in range(50): # Run a few times to be sure
        events, _ = gen.generate(0.0)
        assert len(events) == 0

def test_generate_always_produces_events_when_probability_is_one():
    """
    Tests that notes are always generated when note_probability is 1.0.
    """
    gen = RandomGenerator(note_probability=1.0)
    for _ in range(50):
        events, _ = gen.generate(0.0)
        # It's possible to have no available notes, so we check that
        # if there are notes, one is generated.
        if gen.get_notes_in_range():
            assert len(events) > 0
        else:
            assert len(events) == 0

def test_generate_produces_events_at_roughly_half_probability():
    """
    Tests that notes are generated roughly 50% of the time when probability is 0.5.
    """
    gen = RandomGenerator(note_probability=0.5)
    note_generated_count = 0
    total_runs = 500 # Use a larger sample size for statistical tests
    for _ in range(total_runs):
        events, _ = gen.generate(0.0)
        if len(events) > 0:
            note_generated_count += 1

    # Check if the result is within a reasonable margin of error (e.g., +/- 15%)
    expected_count = total_runs * 0.5
    margin = total_runs * 0.15
    assert expected_count - margin <= note_generated_count <= expected_count + margin
