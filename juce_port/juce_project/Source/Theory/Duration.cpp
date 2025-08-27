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

        float longDuration = baseRate * 2.0f;
        float shortDuration = baseRate / 2.0f;

        static std::mt19937 gen(std::random_device{}());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        float duration;
        // The bias is the probability of choosing the short duration.
        if (dist(gen) < bias)
        {
            duration = shortDuration;
        }
        else
        {
            duration = longDuration;
        }

        juce::String logMessage = "Duration Logic: bias=" + juce::String(bias) + ", rate=" + juce::String(baseRate) + " -> duration=" + juce::String(duration);
        juce::Logger::writeToLog(logMessage);

        return duration;
    }
}
