/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "domain/flare.hpp"

#include <cmath>
#include <string>

namespace fs24::domain
{

namespace
{

constexpr float STEADY_LIMIT_DEG_S = 1.f;
constexpr float POOR_ABOVE_DEG_S   = 2.f;

} // namespace

std::string flare_label(float pitch_rate_deg_s)
{
    const float magnitude = std::abs(pitch_rate_deg_s);
    if (magnitude <= STEADY_LIMIT_DEG_S)
    {
        return "Very good flare";
    }
    std::string label = magnitude > POOR_ABOVE_DEG_S ? "Poor, " : "Good, but ";
    label += pitch_rate_deg_s < 0.f ? "late" : "early";
    return label + " flare";
}

} // namespace fs24::domain
