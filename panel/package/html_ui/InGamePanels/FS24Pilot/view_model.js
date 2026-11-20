/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 * Licensed under the MIT License. See the LICENSE file in the project root.
 */

// View model: turns a `landing` payload (docs/protocol.md) into display text and grade
// classes. Formatting only — every grade comes from the module, nothing is rated here.

const FS24_NEUTRAL = "neutral";
const FS24_WARNING = "warning";

function fs24GradeClass(gradeIndex) {
    return gradeIndex === null || gradeIndex === undefined ? FS24_NEUTRAL : `grade-${gradeIndex}`;
}

function fs24Metric(label, value, tone) {
    return { label, value, tone };
}

function fs24FixedWingRows(landing) {
    const criteria = landing.criteria || {};
    const lastCell =
        landing.bounce_count > 0
            ? fs24Metric("BOUNCES", `${landing.bounce_count}`, FS24_WARNING)
            : fs24Metric("CROSSWIND", `${Math.abs(landing.crosswind_kts)} kts ${landing.crosswind_side}`, FS24_NEUTRAL);
    return [
        [
            fs24Metric("VERTICAL SPEED", `${landing.descent_fpm} fpm`, fs24GradeClass(criteria.descent)),
            fs24Metric("G-FORCE", `${landing.g_force.toFixed(2)} G`, fs24GradeClass(criteria.g_force)),
            fs24Metric("FLOAT", `${landing.float_time_s.toFixed(1)} s`, FS24_NEUTRAL),
        ],
        [
            fs24Metric("IAS", `${landing.ias_kts} kts`, FS24_NEUTRAL),
            fs24Metric("GROUND SPEED", `${landing.ground_speed_kts} kts`, FS24_NEUTRAL),
            lastCell,
        ],
    ];
}

// Above the run-on speed the forward movement is deliberate: shown neutral as ground speed.
function fs24RotorcraftRows(landing) {
    const criteria = landing.criteria || {};
    const travel = landing.run_on
        ? fs24Metric("GROUND SPEED", `${landing.ground_speed_kts} kts`, FS24_NEUTRAL)
        : fs24Metric("DRIFT", `${landing.ground_speed_kts} kts`, fs24GradeClass(criteria.drift));
    return [
        [
            fs24Metric("VERTICAL SPEED", `${landing.descent_fpm} fpm`, fs24GradeClass(criteria.descent)),
            fs24Metric("G-FORCE", `${landing.g_force.toFixed(2)} G`, fs24GradeClass(criteria.g_force)),
        ],
        [travel, fs24Metric("BANK", `${Math.abs(landing.bank_deg).toFixed(1)}°`, fs24GradeClass(criteria.bank))],
        [fs24Metric("YAW RATE", `${Math.abs(landing.yaw_rate_deg_s).toFixed(1)}°/s`, fs24GradeClass(criteria.yaw_rate))],
    ];
}

// What the numbers don't carry: configuration and who was flying.
function fs24ContextParts(landing) {
    const wind = { text: `Wind ${landing.wind_speed_kts} kts`, tone: "" };
    if (landing.is_rotorcraft) {
        return [wind];
    }
    const parts = [{ text: `Flaps ${Math.round(landing.flap_ratio * 100)}%`, tone: "" }, wind];
    if (!landing.gear_down) {
        parts.push({ text: "GEAR NOT DOWN", tone: FS24_WARNING });
    }
    if (landing.autopilot_engaged) {
        parts.push({ text: "AUTOLAND", tone: FS24_WARNING });
    }
    return parts;
}

function fs24RunwayModel(runway) {
    if (!runway) {
        return null;
    }
    const offset = runway.centerline_offset_m;
    return {
        title: `RUNWAY ${runway.ident} — ${runway.length_m} m`,
        lengthM: runway.length_m,
        alongRatio: Math.min(1, Math.max(0, runway.distance_from_threshold_m / runway.length_m)),
        offsetM: offset,
        caption:
            `${runway.distance_from_threshold_m} m past threshold · ` +
            `${Math.abs(offset).toFixed(1)} m ${offset > 0 ? "right" : "left"} of centerline`,
    };
}

function fs24LandingViewModel(landing) {
    return {
        grade: landing.grade,
        gradeClass: fs24GradeClass(landing.grade_index),
        rows: landing.is_rotorcraft ? fs24RotorcraftRows(landing) : fs24FixedWingRows(landing),
        flare: landing.is_rotorcraft ? "" : landing.flare,
        context: fs24ContextParts(landing),
        showRunway: !landing.is_rotorcraft,
        runway: fs24RunwayModel(landing.runway),
        profile: landing.profile,
    };
}
