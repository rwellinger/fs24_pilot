/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include <array>

#include "adapters/simvars/simvar_field.hpp"
#include "domain/frame.hpp"

// The per-frame SimVars and their translation into domain::FrameInput — the only place
// that knows MSFS names, units and sign conventions for the landing data.
namespace fs24::adapters::simvars
{

// One double per field, in FRAME_FIELDS order (SimConnect FLOAT64 data block).
struct RawFrame
{
    double sim_on_ground;
    double gear_center_on_ground;
    double gear_left_on_ground;
    double gear_right_on_ground;
    double skid_front_on_ground;
    double skid_rear_on_ground;
    double agl_m;
    double ias_kts;
    double ground_speed_kts;
    double vertical_speed_fpm;
    double g_force;
    double pitch_deg;
    double pitch_rate_deg_s;
    double bank_deg;
    double yaw_rate_deg_s;
    double heading_true_deg;
    double latitude_deg;
    double longitude_deg;
    double wind_speed_kts;
    double wind_from_true_deg;
    double flap_handle_ratio;
    double flap_ratio;
    double gear_extended_ratio;
    double autopilot_master;
    double slew_active;
    double touchdown_normal_velocity_fps; // the sim's own touchdown capture: the rated descent rate
    double touchdown_pitch_deg;
};

// NOLINTBEGIN(modernize-use-designated-initializers): a table reads better as {name, unit} rows.
inline constexpr std::array<SimVarField, 27> FRAME_FIELDS = {{
    {"SIM ON GROUND", "bool"},
    {"GEAR IS ON GROUND:0", "bool"},
    {"GEAR IS ON GROUND:1", "bool"},
    {"GEAR IS ON GROUND:2", "bool"},
    {"GEAR IS ON GROUND:10", "bool"},
    {"GEAR IS ON GROUND:11", "bool"},
    {"PLANE ALT ABOVE GROUND MINUS CG", "meters"},
    {"AIRSPEED INDICATED", "knots"},
    {"GROUND VELOCITY", "knots"},
    {"VERTICAL SPEED", "feet per minute"},
    {"G FORCE", "gforce"},
    {"PLANE PITCH DEGREES", "degrees"},
    {"ROTATION VELOCITY BODY X", "degrees per second"},
    {"PLANE BANK DEGREES", "degrees"},
    {"ROTATION VELOCITY BODY Y", "degrees per second"},
    {"PLANE HEADING DEGREES TRUE", "degrees"},
    {"PLANE LATITUDE", "degrees"},
    {"PLANE LONGITUDE", "degrees"},
    {"AMBIENT WIND VELOCITY", "knots"},
    {"AMBIENT WIND DIRECTION", "degrees"},
    {"FLAPS HANDLE PERCENT", "percent over 100"},
    {"TRAILING EDGE FLAPS LEFT PERCENT", "percent over 100"},
    {"GEAR TOTAL PCT EXTENDED", "percent over 100"},
    {"AUTOPILOT MASTER", "bool"},
    {"IS SLEW ACTIVE", "bool"},
    {"PLANE TOUCHDOWN NORMAL VELOCITY", "feet per second"},
    {"PLANE TOUCHDOWN PITCH DEGREES", "degrees"},
}};
// NOLINTEND(modernize-use-designated-initializers)

static_assert(sizeof(RawFrame) == FRAME_FIELDS.size() * sizeof(double), "RawFrame must match FRAME_FIELDS");

struct SimState
{
    double sim_time_s = 0;
    bool   running    = false; // "Sim" system event: false in menus and while loading
    bool   paused     = false; // "Pause_EX1": any pause flag, including active pause
};

domain::FrameInput to_frame(const RawFrame &raw, const SimState &state, bool is_rotorcraft);

} // namespace fs24::adapters::simvars
