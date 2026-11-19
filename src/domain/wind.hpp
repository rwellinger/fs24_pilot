/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include <cstdint>

namespace fs24::domain
{

enum class WindCondition : std::uint8_t
{
    Calm,
    Light,
    Steady,
};

struct WindComponents
{
    float headwind_kts  = 0; // + = headwind
    float crosswind_kts = 0; // + = from the right
};

WindComponents wind_components(float wind_speed_kts, float wind_from_deg, float heading_deg);

WindCondition classify_wind(float wind_speed_kts);

} // namespace fs24::domain
