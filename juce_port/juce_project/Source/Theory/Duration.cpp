#include <JuceHeader.h>
#include "Duration.h"
#include <algorithm> // for std::max/min
#include <random>

namespace Duration
{
    float getBiasedDuration(float bias, float baseRate)
    {
        // Ensure bias is in the [0, 1] range
        bias = std::max(0.0f, std::min(1.0f, bias));

    if (bias < 0.5f)
        {
        // Interpolate between Long (rate * 2) and Normal (rate)
        // Map bias from [0.0, 0.5] to a t-value from [0.0, 1.0]
        float t = bias * 2.0f;
        // Invert t because we are going from long to normal as bias increases
        return juce::jmap(1.0f - t, 0.0f, 1.0f, baseRate, baseRate * 2.0f);
        }
        else
        {
        // Interpolate between Normal (rate) and Short (rate / 2)
        // Map bias from [0.5, 1.0] to a t-value from [0.0, 1.0]
        float t = (bias - 0.5f) * 2.0f;
        return juce::jmap(t, 0.0f, 1.0f, baseRate, baseRate / 2.0f);
        }
    }
}
