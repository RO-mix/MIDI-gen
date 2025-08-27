#include <JuceHeader.h>
#include "Duration.h"
#include <algorithm> // for std::max/min

namespace Duration
{
    float lerp(float a, float b, float t)
    {
        return a * (1.0f - t) + b * t;
    }

    float getBiasedDuration(float bias, float baseRate)
    {
        // Ensure bias is in the [0, 1] range
        bias = std::max(0.0f, std::min(1.0f, bias));

        // When bias is 0, duration is longer (rate * 4)
        // When bias is 1, duration is shorter (rate)
        float longDuration = baseRate * 4.0f;
        float shortDuration = baseRate;

        return lerp(longDuration, shortDuration, bias);
    }
}
