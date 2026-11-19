/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include <optional>
#include <string>

#include "domain/grade.hpp"
#include "domain/runway_fix.hpp"
#include "domain/wind.hpp"

namespace fs24::domain
{

// Approach state when crossing 50 ft AGL in the descent.
struct GateCrossing
{
    bool  captured   = false;
    float ias_kts    = 0;
    float fpm        = 0;
    float flap_ratio = 0; // handle position at the gate
};

// One completed landing as it is rated, stored and shown.
struct Landing
{
    bool is_rotorcraft = false;

    // Touchdown kinematics; for bounces those of the hardest main-gear contact.
    float descent_fpm      = 0; // negative
    float g_force          = 0;
    float pitch_deg        = 0;
    float pitch_rate_deg_s = 0;
    float bank_deg         = 0;
    float yaw_rate_deg_s   = 0;
    float ias_kts          = 0;
    float ground_speed_kts = 0;
    float heading_true_deg = 0;
    float float_time_s     = 0;
    int   bounce_count     = 0;

    GateCrossing gate;

    double latitude_deg  = 0;
    double longitude_deg = 0;
    double sim_time_s    = 0;

    float          wind_speed_kts = 0;
    WindComponents wind;
    WindCondition  wind_condition = WindCondition::Calm;

    float flap_ratio        = 0;
    bool  gear_down         = true;
    bool  autopilot_engaged = false;

    std::string flare;
    Grade       grade = Grade::Wasted;
    std::string profile_name;

    std::string              airport_ident;
    std::optional<RunwayFix> runway;
    float                    distance_to_surface_m = -1; // -1 = unknown
};

} // namespace fs24::domain
