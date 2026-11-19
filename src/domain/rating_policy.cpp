/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "domain/rating_policy.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace fs24::domain
{

namespace
{

using Limits = std::array<float, 4>;

// Flight-data-monitoring convention: 2.1 G hard, 2.6 G severe; the gentler steps interpolated.
constexpr Limits FIXED_WING_G_LIMITS = {1.4f, 1.7f, 2.1f, 2.6f};

constexpr Limits ROTORCRAFT_DRIFT_LIMITS_KTS = {1.f, 3.f, 6.f, 10.f};
constexpr Limits ROTORCRAFT_BANK_LIMITS_DEG  = {2.f, 4.f, 7.f, 10.f};
constexpr Limits ROTORCRAFT_YAW_LIMITS_DEG_S = {2.f, 5.f, 10.f, 15.f};
constexpr Limits ROTORCRAFT_G_LIMITS         = {1.15f, 1.3f, 1.5f, 1.8f};

constexpr float MAX_CROSSWIND_KTS        = 30.f;
constexpr float MAX_CROSSWIND_ALLOWANCE  = 0.4f;
constexpr float LIGHT_WIND_ALLOWANCE_CUT = 0.5f;

Grade grade_at(std::size_t index) { return static_cast<Grade>(std::min<std::size_t>(index, GRADE_COUNT - 1)); }

Grade grade_against(float value, const Limits &limits)
{
    for (std::size_t index = 0; index < limits.size(); ++index)
    {
        if (value <= limits[index])
        {
            return grade_at(index);
        }
    }
    return Grade::Wasted;
}

float wind_scale(WindCondition wind)
{
    switch (wind)
    {
    case WindCondition::Calm:
        return 0.f;
    case WindCondition::Light:
        return LIGHT_WIND_ALLOWANCE_CUT;
    case WindCondition::Steady:
        return 1.f;
    }
    return 1.f;
}

} // namespace

float crosswind_allowance(float crosswind_kts, WindCondition wind)
{
    const float capped = std::min(std::abs(crosswind_kts), MAX_CROSSWIND_KTS);
    return 1.f + (capped / MAX_CROSSWIND_KTS) * MAX_CROSSWIND_ALLOWANCE * wind_scale(wind);
}

Grade grade_descent(float descent_fpm, const DescentThresholds &thresholds)
{
    for (std::size_t index = 0; index < thresholds.size(); ++index)
    {
        if (descent_fpm >= static_cast<float>(thresholds[index]))
        {
            return grade_at(index);
        }
    }
    return Grade::Wasted;
}

Grade rate_fixed_wing(const FixedWingTouchdown &touchdown, const DescentThresholds &thresholds)
{
    const float effective_fpm = touchdown.descent_fpm / crosswind_allowance(touchdown.crosswind_kts, touchdown.wind);
    if (effective_fpm > 0.f) // climbing on contact is not a landing anyone got right
    {
        return Grade::Wasted;
    }
    return worse(grade_descent(effective_fpm, thresholds), grade_against(touchdown.g_force, FIXED_WING_G_LIMITS));
}

Grade rate_rotorcraft(const RotorcraftTouchdown &touchdown, const DescentThresholds &thresholds)
{
    Grade worst = grade_descent(touchdown.descent_fpm, thresholds);
    worst       = worse(worst, grade_against(std::abs(touchdown.bank_deg), ROTORCRAFT_BANK_LIMITS_DEG));
    worst       = worse(worst, grade_against(std::abs(touchdown.yaw_rate_deg_s), ROTORCRAFT_YAW_LIMITS_DEG_S));
    worst       = worse(worst, grade_against(touchdown.g_force, ROTORCRAFT_G_LIMITS));
    if (touchdown.drift_kts < ROTORCRAFT_RUN_ON_KTS)
    {
        worst = worse(worst, grade_against(touchdown.drift_kts, ROTORCRAFT_DRIFT_LIMITS_KTS));
    }
    return worst;
}

} // namespace fs24::domain
