/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include <cstdint>
#include <optional>

#include "domain/frame.hpp"
#include "domain/landing.hpp"
#include "domain/time_window.hpp"

namespace fs24::domain
{

inline constexpr float  GATE_AGL_M          = 15.24f; // 50 ft
inline constexpr float  BOUNCE_AGL_LIMIT_FT = 5.f;
inline constexpr double SMOOTHING_WINDOW_S  = 0.5;

enum class TouchdownKind : std::uint8_t
{
    None,
    First,  // first main-gear contact of this approach
    Bounce, // main gear back down after lifting less than BOUNCE_AGL_LIMIT_FT
    Hop,    // back down after a higher hop: ignored, the first contact stands
};

// Landing detection for one approach: smoothing windows, the 50-ft gate, bounce tracking
// and the touchdown captured so far. Fed only with active frames.
class LandingDetector
{
  public:
    // Seeded from the sim rather than assumed airborne: a touchdown is a rising edge of
    // gear contact, and an aircraft armed while on the ground has not landed.
    void arm(bool on_any_gear, bool on_all_gear);

    // A fixed-wing landing completes once the nose gear follows the mains; on bounces the
    // hardest main-gear contact is the one rated.
    std::optional<Landing> step_fixed_wing(const FrameInput &frame);

    // Skids touch down with all contact points at once: captured and completed on the
    // rising edge of full ground contact.
    std::optional<Landing> step_rotorcraft(const FrameInput &frame);

    // Vertical speed from the AGL window — smoother than the sim's reading, which spikes on
    // gear compression. Falls back to the sim's value while the window fills.
    float smoothed_vertical_speed_fpm(const FrameInput &frame) const;

    bool                armed() const { return armed_; }
    bool                has_touchdown() const { return touchdown_.has_value(); }
    const GateCrossing &gate() const { return gate_; }
    int                 bounce_count() const { return bounce_count_; }

  private:
    void          observe(const FrameInput &frame);
    void          track_gate(const FrameInput &frame, float vertical_speed_fpm);
    TouchdownKind classify_main_gear_touchdown(const FrameInput &frame) const;
    void          record_main_gear_touchdown(TouchdownKind kind, const FrameInput &frame);
    void          track_main_gear_lift(const FrameInput &frame);
    void          remember_gear_contact(const FrameInput &frame);
    void          reset_lift_tracking();
    Landing       touchdown_from(const FrameInput &frame) const;
    Landing       complete(Landing landing, const FrameInput &frame);

    TimeWindow agl_window_{SMOOTHING_WINDOW_S};
    TimeWindow g_window_{SMOOTHING_WINDOW_S};
    TimeWindow bank_window_{SMOOTHING_WINDOW_S};
    TimeWindow yaw_rate_window_{SMOOTHING_WINDOW_S};

    bool armed_           = false;
    bool previous_on_any_ = false;
    bool previous_on_all_ = false;

    GateCrossing          gate_;
    float                 previous_agl_m_   = 0;
    float                 previous_ias_kts_ = 0;
    float                 previous_vs_fpm_  = 0;
    std::optional<double> below_gate_since_s_;
    std::optional<float>  float_time_s_;

    std::optional<Landing> touchdown_;
    int                    bounce_count_               = 0;
    float                  worst_fpm_magnitude_        = 0;
    bool                   main_gear_lifted_           = false;
    float                  max_agl_since_touchdown_ft_ = 0;
};

} // namespace fs24::domain
