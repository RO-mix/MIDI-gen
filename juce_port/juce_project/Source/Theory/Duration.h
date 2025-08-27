#pragma once

namespace Duration
{
    // Linearly interpolates the duration between rate and rate * 4
    float getBiasedDuration(float bias, float baseRate);
}
