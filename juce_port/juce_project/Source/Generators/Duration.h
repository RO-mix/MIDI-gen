#pragma once
#include <JuceHeader.h>
#include <vector>

class Duration
{
public:
    /**
     * Generates a random duration based on a bias value.
     * @param bias A value from 0.0 to 1.0 where:
     *             0.0 favors long durations (whole notes)
     *             1.0 favors short durations (32nd notes)
     *             0.5 gives balanced probability
     * @param random Random number generator to use
     * @return Duration value in beats
     */
    static float getProbabilisticDuration(float bias = 0.5f, juce::Random& random = juce::Random::getSystemRandom());

    /**
     * Gets the standard duration values used by the probabilistic generator.
     * @return Array of duration values in beats
     */
    static const std::vector<float>& getDurationValues();

    /**
     * Gets human-readable names for the duration values.
     * @return Array of duration names
     */
    static const std::vector<juce::String>& getDurationNames();

private:
    // Standard musical durations in beats (assuming 4/4 time)
    static const std::vector<float> durationValues_;
    static const std::vector<juce::String> durationNames_;

    // Pre-calculated weights for bias extremes
    static const std::vector<int> weightsLong_;    // Favors long durations
    static const std::vector<int> weightsShort_;   // Favors short durations

    /**
     * Linear interpolation between two values.
     */
    static float lerp(float a, float b, float t);
};