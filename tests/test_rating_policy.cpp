/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include <catch2/catch_amalgamated.hpp>

#include "domain/flare.hpp"
#include "domain/rating_policy.hpp"
#include "domain/wind.hpp"

using namespace fs24::domain;

namespace
{

// turbine_helicopter and light_ga thresholds of the xp_pilot profile list.
constexpr DescentThresholds TURBINE_HELI_FPM{-75, -150, -300, -500};
constexpr DescentThresholds LIGHT_GA_FPM{-100, -200, -300, -500};

RotorcraftTouchdown clean_set_down() { return {-1.f, 0.f, 0.5f, 0.5f, 1.01f}; }

FixedWingTouchdown greaser() { return {-60.f, 1.05f, 0.f, WindCondition::Calm}; }

Grade rate(const RotorcraftTouchdown &touchdown) { return rate_rotorcraft(touchdown, TURBINE_HELI_FPM).overall; }
Grade rate(const FixedWingTouchdown &touchdown) { return rate_fixed_wing(touchdown, LIGHT_GA_FPM).overall; }

} // namespace

TEST_CASE("a clean set-down is still rated BUTTER", "[rating]") { CHECK(rate(clean_set_down()) == Grade::Butter); }

TEST_CASE("drift and bank sink the rating despite a gentle descent", "[rating]")
{
    auto touchdown      = clean_set_down();
    touchdown.drift_kts = 6.f;
    touchdown.bank_deg  = 9.f;
    CHECK(rate(touchdown) == Grade::Hard);
}

TEST_CASE("each rotorcraft criterion on its own can drive the rating", "[rating]")
{
    auto drifting      = clean_set_down();
    drifting.drift_kts = 12.f;
    CHECK(rate(drifting) == Grade::Wasted);

    auto banked     = clean_set_down();
    banked.bank_deg = 11.f;
    CHECK(rate(banked) == Grade::Wasted);

    auto yawing           = clean_set_down();
    yawing.yaw_rate_deg_s = 16.f;
    CHECK(rate(yawing) == Grade::Wasted);

    auto slammed    = clean_set_down();
    slammed.g_force = 1.9f;
    CHECK(rate(slammed) == Grade::Wasted);

    auto dropped        = clean_set_down();
    dropped.descent_fpm = -600.f;
    CHECK(rate(dropped) == Grade::Wasted);
}

TEST_CASE("the worst criterion decides, a good one cannot rescue it", "[rating]")
{
    auto touchdown     = clean_set_down();
    touchdown.bank_deg = 5.f;
    CHECK(rate(touchdown) == Grade::Acceptable);

    touchdown.yaw_rate_deg_s = 16.f;
    CHECK(rate(touchdown) == Grade::Wasted);
}

TEST_CASE("a deliberate run-on landing is not judged as drift", "[rating]")
{
    auto touchdown      = clean_set_down();
    touchdown.drift_kts = 20.f;
    CHECK(rate(touchdown) == Grade::Butter);

    touchdown.drift_kts = ROTORCRAFT_RUN_ON_KTS - 1.f;
    CHECK(rate(touchdown) == Grade::Wasted);
}

TEST_CASE("negative bank and yaw are graded by magnitude", "[rating]")
{
    auto left_bank      = clean_set_down();
    left_bank.bank_deg  = -9.f;
    auto right_bank     = clean_set_down();
    right_bank.bank_deg = 9.f;
    CHECK(rate(left_bank) == rate(right_bank));
}

TEST_CASE("a gentle touchdown in calm air is BUTTER", "[rating]") { CHECK(rate(greaser()) == Grade::Butter); }

TEST_CASE("vertical acceleration can sink a rating the descent rate would pass", "[rating]")
{
    auto touchdown    = greaser();
    touchdown.g_force = 2.2f;
    CHECK(grade_descent(touchdown.descent_fpm, LIGHT_GA_FPM) == Grade::Butter);
    CHECK(rate(touchdown) == Grade::Hard);
}

TEST_CASE("a good g-force cannot rescue a hard descent rate", "[rating]")
{
    auto touchdown        = greaser();
    touchdown.descent_fpm = -600.f;
    CHECK(rate(touchdown) == Grade::Wasted);
}

TEST_CASE("a crosswind buys allowance on the descent rate", "[rating]")
{
    auto calm        = greaser();
    calm.descent_fpm = -210.f;
    CHECK(rate(calm) == Grade::Acceptable);

    auto gusty          = calm;
    gusty.crosswind_kts = 30.f;
    gusty.wind          = WindCondition::Steady;
    CHECK(rate(gusty) == Grade::Great);
}

TEST_CASE("climbing on contact is never a good landing", "[rating]")
{
    auto touchdown        = greaser();
    touchdown.descent_fpm = 20.f;
    CHECK(rate(touchdown) == Grade::Wasted);
}

TEST_CASE("the flare label reads the pitch rate at touchdown", "[rating][flare]")
{
    CHECK(flare_label(0.5f) == "Very good flare");
    CHECK(flare_label(-0.9f) == "Very good flare");
    CHECK(flare_label(1.5f) == "Good, but early flare");
    CHECK(flare_label(-2.5f) == "Poor, late flare");
}

TEST_CASE("wind splits into head- and crosswind, crosswind positive from the right", "[wind]")
{
    const WindComponents from_right = wind_components(10.f, 90.f, 0.f);
    CHECK(from_right.headwind_kts == Catch::Approx(0.f).margin(0.001));
    CHECK(from_right.crosswind_kts == Catch::Approx(10.f));

    const WindComponents head_on = wind_components(10.f, 140.f, 140.f);
    CHECK(head_on.headwind_kts == Catch::Approx(10.f));
    CHECK(head_on.crosswind_kts == Catch::Approx(0.f).margin(0.001));
}

TEST_CASE("wind strength is calm below 3 kts and light below 6 kts", "[wind]")
{
    CHECK(classify_wind(2.9f) == WindCondition::Calm);
    CHECK(classify_wind(3.f) == WindCondition::Light);
    CHECK(classify_wind(5.9f) == WindCondition::Light);
    CHECK(classify_wind(6.f) == WindCondition::Steady);
}

TEST_CASE("The rating names the grade of every criterion", "[rating]")
{
    const Rating fixed = rate_fixed_wing({.descent_fpm = -150.f, .g_force = 1.8f}, LIGHT_GA_FPM);
    CHECK(fixed.criteria.descent == Grade::Great);
    CHECK(fixed.criteria.g_force == Grade::Acceptable);
    CHECK(fixed.overall == Grade::Acceptable);
    CHECK_FALSE(fixed.criteria.bank.has_value());

    const Rating hover =
        rate_rotorcraft({.descent_fpm = -40.f, .drift_kts = 4.f, .bank_deg = -1.f, .g_force = 1.f}, TURBINE_HELI_FPM);
    REQUIRE(hover.criteria.drift.has_value());
    CHECK(*hover.criteria.drift == Grade::Acceptable);
    CHECK(hover.criteria.bank == Grade::Butter);
    CHECK(hover.overall == Grade::Acceptable);

    const Rating run_on = rate_rotorcraft({.descent_fpm = -40.f, .drift_kts = 20.f, .g_force = 1.f}, TURBINE_HELI_FPM);
    CHECK_FALSE(run_on.criteria.drift.has_value());
}
