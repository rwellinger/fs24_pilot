/*
 * Sample `landing` payloads (docs/protocol.md) modelled on landings flown in M03.
 * `runway` is filled where M05 will be able to place the landing.
 */

function fs24Sample(overrides) {
    const base = {
        grade: "GREAT LANDING!", grade_index: 1, profile: "light_ga", is_rotorcraft: false,
        criteria: { descent: 1, g_force: 0, drift: null, bank: null, yaw_rate: null },
        descent_fpm: -103, g_force: 1.12, bounce_count: 0, float_time_s: 4.2,
        ias_kts: 58, ground_speed_kts: 52, pitch_deg: 6.4, bank_deg: 0.6, yaw_rate_deg_s: 0.4,
        flare: "Very good flare", headwind_kts: 6, crosswind_kts: 2, crosswind_side: "right",
        wind_speed_kts: 6, run_on: false, flap_ratio: 1.0, gear_down: true, autopilot_engaged: false,
        airport: "", distance_to_surface_m: -1, runway: null,
    };
    return Object.assign(base, overrides);
}

const FS24_SAMPLES = {
    c172_great: fs24Sample({
        runway: { ident: "14", distance_from_threshold_m: 245, centerline_offset_m: 1.8, length_m: 1730 },
    }),
    c172_butter: fs24Sample({
        grade: "BUTTER!", grade_index: 0, criteria: { descent: 0, g_force: 0, drift: null, bank: null, yaw_rate: null },
        descent_fpm: -19, g_force: 1.02, flare: "Very good flare",
    }),
    kodiak_crosswind: fs24Sample({
        grade: "ACCEPTABLE", grade_index: 2, profile: "turboprop",
        criteria: { descent: 2, g_force: 2, drift: null, bank: null, yaw_rate: null },
        descent_fpm: -356, g_force: 1.45, float_time_s: 2.1, ias_kts: 71, ground_speed_kts: 77, pitch_deg: 1.1,
        flare: "Poor, late flare", headwind_kts: -6, crosswind_kts: -13, crosswind_side: "left", wind_speed_kts: 14,
        flap_ratio: 0.5,
        runway: { ident: "36", distance_from_threshold_m: 512, centerline_offset_m: -4.2, length_m: 1300 },
    }),
    wasted_bounces: fs24Sample({
        grade: "WASTED!", grade_index: 4, criteria: { descent: 4, g_force: 3, drift: null, bank: null, yaw_rate: null },
        descent_fpm: -780, g_force: 2.3, bounce_count: 2, float_time_s: 0.8, ias_kts: 74, ground_speed_kts: 70,
        flare: "Poor, late flare", gear_down: false, autopilot_engaged: true,
    }),
    h145_hover: fs24Sample({
        grade: "BUTTER!", grade_index: 0, profile: "turbine_helicopter", is_rotorcraft: true,
        criteria: { descent: 0, g_force: 0, drift: 0, bank: 0, yaw_rate: 0 },
        descent_fpm: -25, g_force: 1.07, ias_kts: 0, ground_speed_kts: 1, bank_deg: -1.6, yaw_rate_deg_s: 0.8,
        flare: "", wind_speed_kts: 4,
    }),
    h145_run_on: fs24Sample({
        grade: "HARD LANDING!", grade_index: 3, profile: "turbine_helicopter", is_rotorcraft: true,
        criteria: { descent: 3, g_force: 2, drift: null, bank: 0, yaw_rate: 1 },
        descent_fpm: -320, g_force: 1.42, ground_speed_kts: 22, bank_deg: 1.2, yaw_rate_deg_s: 3.1, run_on: true,
        flare: "", wind_speed_kts: 9,
    }),
    none: null,
};
