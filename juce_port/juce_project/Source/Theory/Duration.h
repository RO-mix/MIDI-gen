#pragma once
#include <vector>
#include <random>

namespace Duration
{
    // Returns a duration in beats
    float getProbabilisticDuration(float bias = 0.5f);
}
