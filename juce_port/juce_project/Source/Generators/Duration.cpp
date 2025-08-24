#include "Duration.h"

const std::vector<float> Duration::durationValues_ = {
    4.0f,   // Whole note
    2.0f,   // Half note
    1.0f,   // Quarter note
    0.5f,   // Eighth note
    0.25f,  // 16th note
    0.125f  // 32nd note
};

const std::vector<juce::String> Duration::durationNames_ = {
    "Whole",
    "Half",
    "Quarter",
    "Eighth",
    "16th",
    "32nd"
};

const std::vector<int> Duration::weightsLong_ = {32, 16, 8, 4, 2, 1};
const std::vector<int> Duration::weightsShort_ = {1, 2, 4, 8, 16, 32};

float Duration::getProbabilisticDuration(float bias, juce::Random& random)
{
    // Clamp bias to valid range
    bias = juce::jlimit(0.0f, 1.0f, bias);

    // If bias is exactly 0.5, use uniform distribution
    if (juce::approximatelyEqual(bias, 0.5f))
    {
        int randomIndex = random.nextInt(static_cast<int>(durationValues_.size()));
        return durationValues_[static_cast<size_t>(randomIndex)];
    }

    // Interpolate weights based on the bias
    std::vector<float> finalWeights;
    for (size_t i = 0; i < weightsLong_.size(); ++i)
    {
        float interpolatedWeight = lerp(static_cast<float>(weightsLong_[i]),
                                       static_cast<float>(weightsShort_[i]),
                                       bias);
        finalWeights.push_back(interpolatedWeight);
    }

    // Calculate cumulative weights for selection
    std::vector<float> cumulativeWeights;
    float totalWeight = 0.0f;
    for (float weight : finalWeights)
    {
        totalWeight += weight;
        cumulativeWeights.push_back(totalWeight);
    }

    // Select a duration based on the calculated weights
    float randomValue = random.nextFloat() * totalWeight;
    for (size_t i = 0; i < cumulativeWeights.size(); ++i)
    {
        if (randomValue <= cumulativeWeights[i])
        {
            return durationValues_[i];
        }
    }

    // Fallback (should not reach here)
    return durationValues_.back();
}

const std::vector<float>& Duration::getDurationValues()
{
    return durationValues_;
}

const std::vector<juce::String>& Duration::getDurationNames()
{
    return durationNames_;
}

float Duration::lerp(float a, float b, float t)
{
    return a * (1.0f - t) + b * t;
}