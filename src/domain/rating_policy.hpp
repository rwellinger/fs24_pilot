/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include <array>

#include "domain/grade.hpp"
#include "domain/wind.hpp"

namespace fs24::domain
{

// Negative fpm, gentlest first: upper bounds for BUTTER, GREAT, ACCEPTABLE and HARD.
using DescentThresholds = std::array<int, 4>;

struct FixedWingTouchdown
{
    float         descent_fpm   = 0; // negative
    float         g_force       = 0;
    float         crosswind_kts = 0;
    WindCondition wind          = WindCondition::Steady;
};

// A set-down from the hover always has a near-zero descent rate; what ruins a rotorcraft
// landing is sliding, touching down banked, or letting the tail swing round.
struct RotorcraftTouchdown
{
    float descent_fpm    = 0; // negative
    float drift_kts      = 0; // ground speed at contact
    float bank_deg       = 0;
    float yaw_rate_deg_s = 0;
    float g_force        = 0;
};

// Above this ground speed the forward movement is a deliberate run-on landing, not drift.
inline constexpr float ROTORCRAFT_RUN_ON_KTS = 15.f;

// A crosswind makes the same descent rate better flying: up to 40% allowance in a full
// 30-knot crosswind, none in calm air.
float crosswind_allowance(float crosswind_kts, WindCondition wind);

Grade grade_descent(float descent_fpm, const DescentThresholds &thresholds);

// The worst single criterion decides; a good one cannot rescue a bad one.
Grade rate_fixed_wing(const FixedWingTouchdown &touchdown, const DescentThresholds &thresholds);
Grade rate_rotorcraft(const RotorcraftTouchdown &touchdown, const DescentThresholds &thresholds);

} // namespace fs24::domain
