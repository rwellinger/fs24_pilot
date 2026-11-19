/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include <catch2/catch_amalgamated.hpp>

#include "domain/landing_detector.hpp"
#include "domain/units.hpp"
#include "support.hpp"

using fs24::domain::FrameInput;
using fs24::domain::GATE_AGL_M;
using fs24::domain::LandingDetector;
using fs24::test::Approach;

namespace
{

LandingDetector armed_in_the_air()
{
    LandingDetector detector;
    detector.arm(false, false);
    return detector;
}

auto fixed_wing(LandingDetector &detector)
{
    return [&detector](const FrameInput &frame) { return detector.step_fixed_wing(frame); };
}

// Smoothed vertical speed of a steady descent at `fps`, read just above the ground.
float steady_descent_fpm(double fps)
{
    LandingDetector detector = armed_in_the_air();
    Approach        approach(fps);
    FrameInput      frame;
    while (approach.agl_m > 1.f)
    {
        approach.agl_m -= Approach::DESCENT_MPS / static_cast<float>(fps);
        frame = approach.frame(false, false);
        detector.step_fixed_wing(frame);
    }
    return detector.smoothed_vertical_speed_fpm(frame);
}

} // namespace

TEST_CASE("The 50-ft gate is interpolated between the frames straddling it", "[landing_detector]")
{
    LandingDetector detector = armed_in_the_air();
    FrameInput      above;
    above.agl_m              = GATE_AGL_M + 1.f;
    above.ias_kts            = 70.f;
    above.vertical_speed_fpm = -600.f;
    detector.step_fixed_wing(above);
    CHECK_FALSE(detector.gate().captured);

    FrameInput below         = above;
    below.agl_m              = GATE_AGL_M - 1.f;
    below.ias_kts            = 68.f;
    below.vertical_speed_fpm = -700.f;
    below.flap_handle_ratio  = 0.75f;
    detector.step_fixed_wing(below);

    REQUIRE(detector.gate().captured);
    CHECK(detector.gate().ias_kts == Catch::Approx(69.f));
    CHECK(detector.gate().fpm == Catch::Approx(-650.f));
    CHECK(detector.gate().flap_ratio == Catch::Approx(0.75f));
}

TEST_CASE("The gate is not captured climbing through it or before the detector is armed", "[landing_detector]")
{
    FrameInput low;
    low.agl_m = GATE_AGL_M - 1.f;
    FrameInput high;
    high.agl_m      = GATE_AGL_M + 1.f;
    high.sim_time_s = 1.0;

    LandingDetector climbing = armed_in_the_air();
    climbing.step_fixed_wing(low);
    climbing.step_fixed_wing(high);
    CHECK_FALSE(climbing.gate().captured);

    LandingDetector unarmed;
    unarmed.step_fixed_wing(high);
    unarmed.step_fixed_wing(low);
    CHECK_FALSE(unarmed.gate().captured);
}

TEST_CASE("A fixed-wing landing completes when the nose gear follows the mains", "[landing_detector]")
{
    LandingDetector detector = armed_in_the_air();
    Approach        approach;

    CHECK_FALSE(approach.descend_to_touchdown(fixed_wing(detector)).has_value());
    CHECK(detector.has_touchdown());

    const auto landing = approach.nose_down(fixed_wing(detector));
    REQUIRE(landing.has_value());
    CHECK(landing->descent_fpm == Catch::Approx(-500.f).epsilon(0.02));
    CHECK(landing->bounce_count == 0);
    CHECK(landing->gate.ias_kts == Catch::Approx(65.f));
    CHECK(landing->gate.flap_ratio == Catch::Approx(0.5f));
    CHECK(landing->float_time_s > 0.f);
    CHECK_FALSE(landing->is_rotorcraft);

    // Re-armed for a touch-and-go.
    CHECK(detector.armed());
    CHECK_FALSE(detector.has_touchdown());
}

TEST_CASE("The float time runs from below the gate to the main-gear touchdown", "[landing_detector]")
{
    LandingDetector detector = armed_in_the_air();
    Approach        approach;
    approach.agl_m            = GATE_AGL_M - 0.01f; // already below the gate
    const double below_gate_s = approach.time_s();
    approach.descend_to_touchdown(fixed_wing(detector));
    const double touchdown_s = approach.time_s() - 0.05;

    const auto landing = approach.nose_down(fixed_wing(detector));
    REQUIRE(landing.has_value());
    CHECK(landing->float_time_s == Catch::Approx(touchdown_s - below_gate_s).margin(0.06));
}

TEST_CASE("A low bounce before the nose gear counts as a bounce of the same landing", "[landing_detector]")
{
    LandingDetector detector = armed_in_the_air();
    Approach        approach;
    approach.descend_to_touchdown(fixed_wing(detector));

    CHECK_FALSE(approach.rise_and_settle(fixed_wing(detector), 1.f).has_value()); // ~3 ft
    const auto landing = approach.nose_down(fixed_wing(detector));
    REQUIRE(landing.has_value());
    CHECK(landing->bounce_count == 1);
}

TEST_CASE("A hop above the bounce limit leaves the first touchdown as the rated one", "[landing_detector]")
{
    LandingDetector detector = armed_in_the_air();
    Approach        approach;
    approach.descend_to_touchdown(fixed_wing(detector));
    const float first_fpm = detector.smoothed_vertical_speed_fpm(approach.frame(true, false));

    CHECK_FALSE(approach.rise_and_settle(fixed_wing(detector), 3.f).has_value()); // ~10 ft
    const auto landing = approach.nose_down(fixed_wing(detector));
    REQUIRE(landing.has_value());
    CHECK(landing->bounce_count == 0);
    CHECK(landing->descent_fpm == Catch::Approx(first_fpm).epsilon(0.05));
}

TEST_CASE("Of several bounces the hardest touchdown is kept and every bounce is counted", "[landing_detector]")
{
    LandingDetector detector = armed_in_the_air();
    Approach        approach;
    approach.descend_to_touchdown(fixed_wing(detector));
    approach.rise_and_settle(fixed_wing(detector), 0.5f);

    // Second bounce: 3 m/s (~590 fpm) from just below the bounce limit, a full smoothing
    // window long — the harder contact.
    for (int frame = 10; frame > 0; --frame)
    {
        approach.agl_m = 0.15f * static_cast<float>(frame);
        detector.step_fixed_wing(approach.frame(false, false));
    }
    approach.agl_m = 0.f;
    detector.step_fixed_wing(approach.frame(true, false));
    CHECK(detector.bounce_count() == 2);

    const auto landing = approach.nose_down(fixed_wing(detector));
    REQUIRE(landing.has_value());
    CHECK(landing->bounce_count == 2);
    CHECK(landing->descent_fpm < -550.f); // harder than the first ~500 fpm contact
}

TEST_CASE("Nose gear contact without a main-gear touchdown completes nothing", "[landing_detector]")
{
    LandingDetector detector = armed_in_the_air();
    FrameInput      all_down = fs24::test::parked();
    detector.arm(true, false); // mains already down when armed: not a touchdown
    CHECK_FALSE(detector.step_fixed_wing(all_down).has_value());
}

TEST_CASE("Skids complete a rotorcraft landing on the rising edge of full contact", "[landing_detector]")
{
    LandingDetector detector = armed_in_the_air();
    FrameInput      one_skid;
    one_skid.on_any_gear = true;
    CHECK_FALSE(detector.step_rotorcraft(one_skid).has_value());

    const auto landing = detector.step_rotorcraft(fs24::test::parked());
    REQUIRE(landing.has_value());
    CHECK(landing->is_rotorcraft);
    CHECK(landing->bounce_count == 0);
    CHECK_FALSE(detector.step_rotorcraft(fs24::test::parked()).has_value());
}

TEST_CASE("Vertical speed comes from the AGL window once it spans time", "[landing_detector]")
{
    LandingDetector detector = armed_in_the_air();
    FrameInput      frame;
    frame.vertical_speed_fpm = -480.f;
    CHECK(detector.smoothed_vertical_speed_fpm(frame) == Catch::Approx(-480.f));

    // 1 m/s down at 20 fps.
    for (int i = 0; i <= 20; ++i)
    {
        frame.agl_m      = 20.f - static_cast<float>(i) * 0.05f;
        frame.sim_time_s = static_cast<double>(i) * 0.05;
        detector.step_fixed_wing(frame);
    }
    CHECK(detector.smoothed_vertical_speed_fpm(frame) ==
          Catch::Approx(-fs24::domain::units::METERS_PER_S_TO_FPM).margin(1.f));
}

TEST_CASE("The smoothed vertical speed does not depend on the frame rate", "[landing_detector]")
{
    const float at_20_fps  = steady_descent_fpm(20.0);
    const float at_60_fps  = steady_descent_fpm(60.0);
    const float at_120_fps = steady_descent_fpm(120.0);

    CHECK(at_20_fps == Catch::Approx(-500.f).epsilon(0.02));
    CHECK(at_60_fps == Catch::Approx(at_20_fps).epsilon(0.02));
    CHECK(at_120_fps == Catch::Approx(at_20_fps).epsilon(0.02));
}

TEST_CASE("The touchdown records real pitch attitude next to the pitch rate", "[landing_detector]")
{
    LandingDetector detector = armed_in_the_air();
    Approach        approach;
    auto            sink = [&detector](FrameInput frame)
    {
        frame.pitch_deg        = 5.5f;
        frame.pitch_rate_deg_s = -1.5f;
        return detector.step_fixed_wing(frame);
    };
    approach.descend_to_touchdown(sink);
    const auto landing = approach.nose_down(sink);
    REQUIRE(landing.has_value());
    CHECK(landing->pitch_deg == Catch::Approx(5.5f));
    CHECK(landing->pitch_rate_deg_s == Catch::Approx(-1.5f));
    CHECK(landing->flare == "Good, but late flare");
}
