/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include <optional>

#include "domain/flight_phase_machine.hpp"
#include "domain/frame.hpp"
#include "domain/landing.hpp"
#include "domain/landing_detector.hpp"

// Synthetic sim frames for the landing tests.
namespace fs24::test
{

inline bool completed(const std::optional<domain::Landing> &landing) { return landing.has_value(); }
inline bool completed(const domain::PhaseStep &step) { return step.transition != domain::PhaseTransition::None; }

inline domain::FrameInput parked()
{
    domain::FrameInput frame;
    frame.on_any_gear = true;
    frame.on_all_gear = true;
    return frame;
}

inline domain::FrameInput takeoff_roll()
{
    domain::FrameInput frame = parked();
    frame.ground_speed_kts   = 40.f;
    return frame;
}

inline domain::FrameInput climbing()
{
    domain::FrameInput frame = parked();
    frame.on_any_gear        = false;
    frame.on_all_gear        = false;
    frame.ground_speed_kts   = 80.f;
    frame.agl_m              = 100.f;
    frame.latitude_deg       = 46.9;
    frame.longitude_deg      = 7.5;
    return frame;
}

// A stabilised fixed-wing approach: 500 fpm down at 65 kts, flaps half, at a given frame rate.
class Approach
{
  public:
    static constexpr float DESCENT_MPS = 2.54f; // 500 fpm

    // Starts a whole number of frames above the ground, so the touchdown frame descends a full step.
    explicit Approach(double fps = 20.0, double start_s = 1000.0)
        : agl_m(step_m(fps) * START_FRAMES), frame_s_(1.0 / fps), t_(start_s)
    {
    }

    domain::FrameInput frame(bool on_any, bool on_all)
    {
        domain::FrameInput f = climbing();
        f.sim_time_s         = t_;
        f.agl_m              = agl_m;
        f.on_any_gear        = on_any;
        f.on_all_gear        = on_all;
        f.ias_kts            = 65.f;
        f.ground_speed_kts   = 64.f;
        f.vertical_speed_fpm = -500.f;
        f.flap_handle_ratio  = 0.5f;
        f.heading_true_deg   = 140.f;
        t_ += frame_s_;
        return f;
    }

    // Steps `sink` with every frame down to main-gear contact; returns the last result.
    template <typename Sink> auto descend_to_touchdown(Sink &&sink)
    {
        decltype(sink(frame(false, false))) last{};
        const float                         step = DESCENT_MPS * static_cast<float>(frame_s_);
        while (agl_m > 0.f)
        {
            agl_m -= step;
            if (agl_m < step / 2.f) // float residue: this is the contact frame
            {
                agl_m = 0.f;
            }
            last = sink(frame(agl_m <= 0.f, false));
        }
        return last;
    }

    template <typename Sink> auto rise_and_settle(Sink &&sink, float peak_m)
    {
        for (agl_m = 0.1f; agl_m < peak_m; agl_m += 0.1f)
        {
            sink(frame(false, false));
        }
        agl_m = 0.f;
        return sink(frame(true, false));
    }

    // Nose gear down, then rolling on all gear until the sink reports a result or the
    // impact window has surely passed; returns that result.
    template <typename Sink> auto nose_down(Sink &&sink)
    {
        agl_m                = 0.f;
        auto      last       = sink(frame(true, true));
        const int max_frames = static_cast<int>((domain::IMPACT_WINDOW_S + 0.2) / frame_s_);
        for (int i = 0; i < max_frames && !completed(last); ++i)
        {
            last = sink(frame(true, true));
        }
        return last;
    }

    double time_s() const { return t_; }

    float agl_m;

  private:
    static constexpr float START_FRAMES = 240.f; // ~30 m at 20 fps

    static float step_m(double fps) { return DESCENT_MPS / static_cast<float>(fps); }

    double frame_s_;
    double t_;
};

} // namespace fs24::test
