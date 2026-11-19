/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

namespace fs24::domain
{

// Everything the landing logic reads from the sim in one frame, in explicitly named
// units. The simvars adapter is the only place that knows where each value comes from.
struct FrameInput
{
    // Monotonic sim time; drives the smoothing windows and all durations.
    double sim_time_s = 0;
    // False while paused, in replay or slewing: such frames carry no motion and are ignored.
    bool active = true;

    bool on_any_gear = false; // at least one main contact point on the ground
    bool on_all_gear = false; // every contact point on the ground (nose gear / both skids)

    float agl_m              = 0;
    float ias_kts            = 0;
    float ground_speed_kts   = 0;
    float vertical_speed_fpm = 0; // the sim's own reading; fallback while the AGL window fills
    float g_force            = 0;

    float pitch_deg        = 0; // attitude, + = nose up
    float pitch_rate_deg_s = 0; // + = nose rising
    float bank_deg         = 0; // + = right wing down
    float yaw_rate_deg_s   = 0;
    float heading_true_deg = 0;

    double latitude_deg  = 0;
    double longitude_deg = 0;

    float wind_speed_kts     = 0;
    float wind_from_true_deg = 0;

    float flap_handle_ratio = 0; // 0 = up, 1 = full
    float flap_ratio        = 0; // surfaces, 0 = clean, 1 = full
    bool  gear_down         = true;
    bool  autopilot_engaged = false;
};

} // namespace fs24::domain
