#include "Duration.h"
#include <numeric>

namespace Duration
{
    static const std::vector<float> DURATION_VALUES = { 4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 0.125f };
    static const std::vector<double> WEIGHTS_LONG = { 32, 16, 8, 4, 2, 1 };
    static const std::vector<double> WEIGHTS_SHORT = { 1, 2, 4, 8, 16, 32 };

    double lerp(double a, double b, double t)
    {
        return a * (1.0 - t) + b * t;
    }

    float getProbabilisticDuration(float bias)
    {
        if (bias < 0.0f || bias > 1.0f)
        {
            // Handle error or clamp bias, for now let's clamp
            bias = std::max(0.0f, std::min(1.0f, bias));
        }

        std::vector<double> final_weights;
        for (size_t i = 0; i < WEIGHTS_LONG.size(); ++i)
        {
            final_weights.push_back(lerp(WEIGHTS_LONG[i], WEIGHTS_SHORT[i], bias));
        }

        std::discrete_distribution<> dist(final_weights.begin(), final_weights.end());
        std::mt19937 gen(std::random_device{}());

        return DURATION_VALUES[dist(gen)];
    }
}
