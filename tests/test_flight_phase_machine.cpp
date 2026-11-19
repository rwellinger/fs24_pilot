/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include <catch2/catch_amalgamated.hpp>

#include "domain/flight_phase_machine.hpp"
#include "support.hpp"

using namespace fs24::domain;
using fs24::test::Approach;

namespace
{

constexpr double FRAME_S = 0.05; // 20 fps

class Driver
{
  public:
    explicit Driver(bool rotorcraft = false) : rotorcraft_(rotorcraft) {}

    PhaseStep step(FrameInput frame)
    {
        frame.sim_time_s = t_;
        t_ += FRAME_S;
        return machine.step(frame, rotorcraft_);
    }

    // Holds one frame for a stretch of sim time; returns the first transition it causes.
    PhaseTransition hold(const FrameInput &frame, double seconds)
    {
        PhaseTransition first = PhaseTransition::None;
        for (double held = 0.0; held < seconds; held += FRAME_S)
        {
            const PhaseTransition transition = step(frame).transition;
            if (first == PhaseTransition::None)
            {
                first = transition;
            }
        }
        return first;
    }

    FlightPhaseMachine machine;

  private:
    bool   rotorcraft_;
    double t_ = 0.0;
};

FrameInput hovering()
{
    FrameInput frame  = fs24::test::parked();
    frame.on_any_gear = false;
    frame.on_all_gear = false;
    frame.agl_m       = 2.f;
    return frame;
}

Driver airborne_helicopter()
{
    Driver driver(true);
    driver.hold(fs24::test::parked(), ROTORCRAFT_GROUND_SETTLE_S + 0.1);
    REQUIRE(driver.hold(hovering(), 2.5) == PhaseTransition::LiftedOffRotorcraft);
    return driver;
}

} // namespace

TEST_CASE("A fixed-wing flight runs Idle, Rolling, Airborne and Landed", "[phases]")
{
    Driver driver;
    CHECK(driver.step(fs24::test::parked()).transition == PhaseTransition::None);
    CHECK(driver.step(fs24::test::takeoff_roll()).transition == PhaseTransition::StartedRoll);
    CHECK(driver.step(fs24::test::climbing()).transition == PhaseTransition::TookOff);
    CHECK(driver.machine.phase() == FlightPhase::Airborne);
    CHECK(driver.machine.detector().armed());

    Approach approach(20.0, 100.0);
    auto     sink = [&driver](const FrameInput &frame) { return driver.machine.step(frame, false); };
    CHECK(approach.descend_to_touchdown(sink).transition == PhaseTransition::None);
    const PhaseStep landed = approach.nose_down(sink);
    REQUIRE(landed.transition == PhaseTransition::Landed);
    REQUIRE(landed.landing.has_value());
    CHECK(landed.landing->descent_fpm < 0.f);
    CHECK(driver.machine.phase() == FlightPhase::Landed);
}

TEST_CASE("A fixed-wing recording only starts from a ground roll", "[phases]")
{
    Driver     driver;
    FrameInput slow_taxi       = fs24::test::parked();
    slow_taxi.ground_speed_kts = 15.f;
    CHECK(driver.hold(slow_taxi, 5.0) == PhaseTransition::None);
    CHECK(driver.machine.phase() == FlightPhase::Idle);
}

TEST_CASE("A rejected takeoff stays Rolling", "[phases]")
{
    Driver driver;
    driver.step(fs24::test::takeoff_roll());
    CHECK(driver.hold(fs24::test::parked(), 5.0) == PhaseTransition::None);
    CHECK(driver.machine.phase() == FlightPhase::Rolling);
}

TEST_CASE("A touch-and-go goes airborne again and re-arms the detector", "[phases]")
{
    Driver driver;
    driver.step(fs24::test::takeoff_roll());
    driver.step(fs24::test::climbing());
    Approach approach(20.0, 100.0);
    auto     sink = [&driver](const FrameInput &frame) { return driver.machine.step(frame, false); };
    approach.descend_to_touchdown(sink);
    REQUIRE(approach.nose_down(sink).transition == PhaseTransition::Landed);

    FrameInput rolling_on_runway       = fs24::test::parked();
    rolling_on_runway.ground_speed_kts = 60.f;
    CHECK(driver.step(rolling_on_runway).transition == PhaseTransition::None);
    CHECK(driver.step(fs24::test::climbing()).transition == PhaseTransition::TookOffAgain);
    CHECK(driver.machine.detector().armed());
    CHECK_FALSE(driver.machine.detector().has_touchdown());
}

TEST_CASE("No landing is reported while the aircraft never touches down", "[phases]")
{
    Driver driver;
    driver.step(fs24::test::takeoff_roll());
    driver.step(fs24::test::climbing());
    FrameInput cruise = fs24::test::climbing();
    cruise.agl_m      = 1500.f;
    CHECK(driver.hold(cruise, 30.0) == PhaseTransition::None);
}

TEST_CASE("A helicopter starts only after settling and a confirmed lift-off", "[phases]")
{
    SECTION("the drop after placement does not count as a flight")
    {
        Driver driver(true);
        CHECK(driver.hold(hovering(), 5.0) == PhaseTransition::None);
        CHECK(driver.machine.phase() == FlightPhase::Idle);
    }

    SECTION("a lift-off after standing on the skids starts the flight")
    {
        Driver driver(true);
        driver.hold(fs24::test::parked(), ROTORCRAFT_GROUND_SETTLE_S + 0.1);
        REQUIRE(driver.machine.settled_since_placed());
        CHECK(driver.hold(hovering(), 1.0) == PhaseTransition::None);
        CHECK(driver.hold(hovering(), 2.0) == PhaseTransition::LiftedOffRotorcraft);
        CHECK(driver.machine.detector().armed());
    }

    SECTION("a reset requires settling again")
    {
        Driver driver(true);
        driver.hold(fs24::test::parked(), ROTORCRAFT_GROUND_SETTLE_S + 0.1);
        driver.machine.reset();
        CHECK(driver.hold(hovering(), 5.0) == PhaseTransition::None);
    }
}

TEST_CASE("A helicopter lands on full skid contact and goes again by skid contact, not AGL", "[phases]")
{
    Driver driver = airborne_helicopter();

    FrameInput one_skid  = hovering();
    one_skid.on_any_gear = true;
    one_skid.agl_m       = 0.f;
    CHECK(driver.step(one_skid).transition == PhaseTransition::None);

    const PhaseStep landed = driver.step(fs24::test::parked());
    REQUIRE(landed.transition == PhaseTransition::Landed);
    REQUIRE(landed.landing.has_value());
    CHECK(landed.landing->is_rotorcraft);

    driver.hold(fs24::test::parked(), 1.0);
    CHECK(driver.hold(hovering(), 1.0) == PhaseTransition::None);
    CHECK(driver.hold(hovering(), 1.5) == PhaseTransition::TookOffAgain);
}

TEST_CASE("A single frame off the ground does not confirm a rotorcraft lift-off", "[phases]")
{
    Driver driver(true);
    driver.hold(fs24::test::parked(), ROTORCRAFT_GROUND_SETTLE_S + 0.1);
    driver.step(hovering());
    CHECK(driver.step(fs24::test::parked()).transition == PhaseTransition::None);
    CHECK(driver.hold(hovering(), 1.5) == PhaseTransition::None);
}

TEST_CASE("A long gap between frames is not counted as ground time", "[phases]")
{
    FlightPhaseMachine machine;
    FrameInput         frame = fs24::test::parked();
    machine.step(frame, true);
    frame.sim_time_s = 60.0; // e.g. after a reposition
    machine.step(frame, true);
    CHECK_FALSE(machine.settled_since_placed());
}
