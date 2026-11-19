/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "domain/wind.hpp"

#include <cmath>
#include <numbers>

namespace fs24::domain
{

namespace
{

constexpr float CALM_BELOW_KTS  = 3.f;
constexpr float LIGHT_BELOW_KTS = 6.f;

} // namespace

WindComponents wind_components(float wind_speed_kts, float wind_from_deg, float heading_deg)
{
    const float relative_rad = (wind_from_deg - heading_deg) * std::numbers::pi_v<float> / 180.f;
    return {.headwind_kts  = wind_speed_kts * std::cos(relative_rad),
            .crosswind_kts = wind_speed_kts * std::sin(relative_rad)};
}

WindCondition classify_wind(float wind_speed_kts)
{
    if (wind_speed_kts < CALM_BELOW_KTS)
    {
        return WindCondition::Calm;
    }
    return wind_speed_kts < LIGHT_BELOW_KTS ? WindCondition::Light : WindCondition::Steady;
}

} // namespace fs24::domain
