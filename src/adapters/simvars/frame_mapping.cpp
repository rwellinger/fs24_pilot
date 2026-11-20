/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "adapters/simvars/frame_mapping.hpp"

namespace fs24::adapters::simvars
{

namespace
{

constexpr float FEET_PER_SECOND_TO_FPM = 60.f;

// MSFS reports pitch positive nose-down (to verify in the sim against PLANE TOUCHDOWN PITCH DEGREES);
// the domain wants nose-up positive. The body-X rotation follows the same convention.
constexpr float PITCH_SIGN = -1.f;

bool is_set(double flag) { return flag != 0.0; }

// Tricycle and taildragger alike: the landing completes once all three legs carry weight.
bool all_wheels_down(const RawFrame &raw)
{
    return is_set(raw.gear_center_on_ground) && is_set(raw.gear_left_on_ground) && is_set(raw.gear_right_on_ground);
}

// Skid helicopters may not map their skids to gear indices at all; then the first ground
// contact is the set-down.
bool skids_down(const RawFrame &raw)
{
    const bool any_gear_flag = is_set(raw.gear_left_on_ground) || is_set(raw.gear_right_on_ground) ||
                               is_set(raw.skid_front_on_ground) || is_set(raw.skid_rear_on_ground);
    if (!any_gear_flag)
    {
        return is_set(raw.sim_on_ground);
    }
    return (is_set(raw.gear_left_on_ground) && is_set(raw.gear_right_on_ground)) ||
           (is_set(raw.skid_front_on_ground) && is_set(raw.skid_rear_on_ground));
}

} // namespace

domain::FrameInput to_frame(const RawFrame &raw, const SimState &state, bool is_rotorcraft)
{
    domain::FrameInput frame;
    frame.sim_time_s = state.sim_time_s;
    frame.active     = state.running && !state.paused && !is_set(raw.slew_active);

    frame.on_any_gear = is_set(raw.sim_on_ground) || is_set(raw.gear_left_on_ground) ||
                        is_set(raw.gear_right_on_ground) || is_set(raw.gear_center_on_ground);
    frame.on_all_gear = frame.on_any_gear && (is_rotorcraft ? skids_down(raw) : all_wheels_down(raw));

    frame.agl_m              = static_cast<float>(raw.agl_m);
    frame.ias_kts            = static_cast<float>(raw.ias_kts);
    frame.ground_speed_kts   = static_cast<float>(raw.ground_speed_kts);
    frame.vertical_speed_fpm = static_cast<float>(raw.vertical_speed_fpm);
    frame.g_force            = static_cast<float>(raw.g_force);
    frame.sim_touchdown_descent_fpm =
        -static_cast<float>(raw.touchdown_normal_velocity_fps) * FEET_PER_SECOND_TO_FPM; // reported downward-positive
    frame.pitch_deg          = PITCH_SIGN * static_cast<float>(raw.pitch_deg);
    frame.pitch_rate_deg_s   = PITCH_SIGN * static_cast<float>(raw.pitch_rate_deg_s);
    frame.bank_deg           = static_cast<float>(raw.bank_deg);
    frame.yaw_rate_deg_s     = static_cast<float>(raw.yaw_rate_deg_s);
    frame.heading_true_deg   = static_cast<float>(raw.heading_true_deg);
    frame.latitude_deg       = raw.latitude_deg;
    frame.longitude_deg      = raw.longitude_deg;
    frame.wind_speed_kts     = static_cast<float>(raw.wind_speed_kts);
    frame.wind_from_true_deg = static_cast<float>(raw.wind_from_true_deg);
    frame.flap_handle_ratio  = static_cast<float>(raw.flap_handle_ratio);
    frame.flap_ratio         = static_cast<float>(raw.flap_ratio);
    frame.gear_down          = raw.gear_extended_ratio >= 0.99;
    frame.autopilot_engaged  = is_set(raw.autopilot_master);
    return frame;
}

} // namespace fs24::adapters::simvars
