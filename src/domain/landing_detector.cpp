/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "domain/landing_detector.hpp"

#include <cmath>
#include <optional>
#include <utility>

#include "domain/flare.hpp"
#include "domain/units.hpp"
#include "domain/wind.hpp"

namespace fs24::domain
{

void LandingDetector::arm(bool on_any_gear, bool on_all_gear)
{
    *this            = LandingDetector{};
    armed_           = true;
    previous_on_any_ = on_any_gear;
    previous_on_all_ = on_all_gear;
}

std::optional<Landing> LandingDetector::step_fixed_wing(const FrameInput &frame)
{
    observe(frame);
    track_gate(frame, smoothed_vertical_speed_fpm(frame));

    const TouchdownKind kind = classify_main_gear_touchdown(frame);
    record_main_gear_touchdown(kind, frame);
    track_main_gear_lift(frame);

    const bool nose_gear_settled = armed_ && touchdown_ && !previous_on_all_ && frame.on_all_gear;
    if (!nose_gear_settled)
    {
        remember_gear_contact(frame);
        return std::nullopt;
    }
    Landing landing      = *touchdown_;
    landing.bounce_count = bounce_count_;
    return complete(std::move(landing), frame);
}

std::optional<Landing> LandingDetector::step_rotorcraft(const FrameInput &frame)
{
    observe(frame);
    const bool skids_touched_down = armed_ && !previous_on_all_ && frame.on_all_gear;
    if (!skids_touched_down)
    {
        remember_gear_contact(frame);
        return std::nullopt;
    }
    Landing landing       = touchdown_from(frame);
    landing.is_rotorcraft = true;
    return complete(std::move(landing), frame);
}

float LandingDetector::smoothed_vertical_speed_fpm(const FrameInput &frame) const
{
    const double span_s = agl_window_.time_span_s();
    if (span_s <= 0.0)
    {
        return frame.vertical_speed_fpm;
    }
    // The window average sits at its mid-time, so the slope is taken over half the span.
    const auto half_span_s = static_cast<float>(span_s / 2.0);
    return (frame.agl_m - agl_window_.average()) / half_span_s * units::METERS_PER_S_TO_FPM;
}

void LandingDetector::observe(const FrameInput &frame)
{
    agl_window_.push(frame.agl_m, frame.sim_time_s);
    g_window_.push(frame.g_force, frame.sim_time_s);
    bank_window_.push(frame.bank_deg, frame.sim_time_s);
    yaw_rate_window_.push(frame.yaw_rate_deg_s, frame.sim_time_s);
}

// Interpolated between the two frames straddling the gate: at 20 fps and 700 fpm a single
// frame spans roughly 3 ft, enough to skew the reading.
void LandingDetector::track_gate(const FrameInput &frame, float vertical_speed_fpm)
{
    const bool crossing_gate = armed_ && !gate_.captured && previous_agl_m_ > GATE_AGL_M && frame.agl_m <= GATE_AGL_M;
    if (crossing_gate)
    {
        const float descent = previous_agl_m_ - frame.agl_m;
        const float ratio   = descent > 0.f ? (previous_agl_m_ - GATE_AGL_M) / descent : 0.f;
        gate_.ias_kts       = previous_ias_kts_ + (frame.ias_kts - previous_ias_kts_) * ratio;
        gate_.fpm           = previous_vs_fpm_ + (vertical_speed_fpm - previous_vs_fpm_) * ratio;
        // The handle rather than the surfaces: a selection made at the gate is a late change
        // even while the surfaces are still travelling.
        gate_.flap_ratio = frame.flap_handle_ratio;
        gate_.captured   = true;
    }
    previous_agl_m_   = frame.agl_m;
    previous_ias_kts_ = frame.ias_kts;
    previous_vs_fpm_  = vertical_speed_fpm;

    if (armed_ && frame.agl_m <= GATE_AGL_M && !below_gate_since_s_)
    {
        below_gate_since_s_ = frame.sim_time_s;
    }
}

// A hop high enough to be a mini touch-and-go keeps the first touchdown as the rating; a
// real touch-and-go (above 50 ft) produces a landing of its own anyway.
TouchdownKind LandingDetector::classify_main_gear_touchdown(const FrameInput &frame) const
{
    if (!armed_ || previous_on_any_ || !frame.on_any_gear)
    {
        return TouchdownKind::None;
    }
    if (!touchdown_)
    {
        return TouchdownKind::First;
    }
    if (main_gear_lifted_ && max_agl_since_touchdown_ft_ < BOUNCE_AGL_LIMIT_FT)
    {
        return TouchdownKind::Bounce;
    }
    return TouchdownKind::Hop;
}

void LandingDetector::record_main_gear_touchdown(TouchdownKind kind, const FrameInput &frame)
{
    switch (kind)
    {
    case TouchdownKind::None:
        return;
    case TouchdownKind::Hop:
        reset_lift_tracking();
        return;
    case TouchdownKind::Bounce:
        ++bounce_count_;
        reset_lift_tracking();
        break;
    case TouchdownKind::First:
        break;
    }

    if (below_gate_since_s_ && !float_time_s_)
    {
        float_time_s_ = static_cast<float>(frame.sim_time_s - *below_gate_since_s_);
    }
    Landing     candidate     = touchdown_from(frame);
    const float fpm_magnitude = std::abs(candidate.descent_fpm);
    if (kind == TouchdownKind::Bounce && fpm_magnitude <= worst_fpm_magnitude_)
    {
        return;
    }
    touchdown_           = std::move(candidate);
    worst_fpm_magnitude_ = fpm_magnitude;
}

void LandingDetector::track_main_gear_lift(const FrameInput &frame)
{
    if (!touchdown_ || frame.on_any_gear)
    {
        return;
    }
    main_gear_lifted_  = true;
    const float agl_ft = frame.agl_m * units::METERS_TO_FEET;
    if (agl_ft > max_agl_since_touchdown_ft_)
    {
        max_agl_since_touchdown_ft_ = agl_ft;
    }
}

void LandingDetector::remember_gear_contact(const FrameInput &frame)
{
    previous_on_any_ = frame.on_any_gear;
    previous_on_all_ = frame.on_all_gear;
}

void LandingDetector::reset_lift_tracking()
{
    main_gear_lifted_           = false;
    max_agl_since_touchdown_ft_ = 0.f;
}

Landing LandingDetector::touchdown_from(const FrameInput &frame) const
{
    Landing landing;
    landing.descent_fpm       = smoothed_vertical_speed_fpm(frame);
    landing.g_force           = g_window_.average();
    landing.pitch_deg         = frame.pitch_deg;
    landing.pitch_rate_deg_s  = frame.pitch_rate_deg_s;
    landing.bank_deg          = bank_window_.average();
    landing.yaw_rate_deg_s    = yaw_rate_window_.average();
    landing.ias_kts           = frame.ias_kts;
    landing.ground_speed_kts  = frame.ground_speed_kts;
    landing.heading_true_deg  = frame.heading_true_deg;
    landing.float_time_s      = float_time_s_.value_or(0.f);
    landing.gate              = gate_;
    landing.latitude_deg      = frame.latitude_deg;
    landing.longitude_deg     = frame.longitude_deg;
    landing.sim_time_s        = frame.sim_time_s;
    landing.wind_speed_kts    = frame.wind_speed_kts;
    landing.wind              = wind_components(frame.wind_speed_kts, frame.wind_from_true_deg, frame.heading_true_deg);
    landing.wind_condition    = classify_wind(frame.wind_speed_kts);
    landing.flap_ratio        = frame.flap_ratio;
    landing.gear_down         = frame.gear_down;
    landing.autopilot_engaged = frame.autopilot_engaged;
    landing.flare             = flare_label(frame.pitch_rate_deg_s);
    return landing;
}

Landing LandingDetector::complete(Landing landing, const FrameInput &frame)
{
    arm(frame.on_any_gear, frame.on_all_gear);
    return landing;
}

} // namespace fs24::domain
