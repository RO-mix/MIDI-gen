import random

# Durations in musical terms and their corresponding values in beats (assuming 4/4 time)
# Using a dictionary to keep the name and value associated
DURATIONS = {
    "Whole": 4.0,
    "Half": 2.0,
    "Quarter": 1.0,
    "Eighth": 0.5,
    "16th": 0.25,
    "32nd": 0.125,
}

# Convert dict to a list of tuples for easier processing: (name, value)
DURATION_ITEMS = list(DURATIONS.items())

# Pre-calculated weights for the two extremes of the bias
# Weights are designed to be simple powers of 2 for a strong effect
WEIGHTS_LONG =  [32, 16, 8, 4, 2, 1]
WEIGHTS_SHORT = [1, 2, 4, 8, 16, 32]

def lerp(a, b, t):
    """Linear interpolation between a and b at ratio t."""
    return a * (1 - t) + b * t

def get_probalistic_duration(bias: float = 0.5) -> float:
    """
    Selects a random duration based on a bias.

    Args:
        bias (float): A value from 0.0 to 1.0.
                      0.0 favors long durations.
                      1.0 favors short durations.
                      0.5 gives a balanced probability.

    Returns:
        float: A duration value in beats.
    """
    if not (0.0 <= bias <= 1.0):
        raise ValueError("Bias must be between 0.0 and 1.0")

    # Interpolate weights based on the bias
    final_weights = [lerp(w_long, w_short, bias) for w_long, w_short in zip(WEIGHTS_LONG, WEIGHTS_SHORT)]

    # Get only the duration values for the random choice
    duration_values = [item[1] for item in DURATION_ITEMS]

    # Choose a duration based on the calculated weights
    chosen_duration = random.choices(duration_values, weights=final_weights, k=1)[0]

    return chosen_duration
