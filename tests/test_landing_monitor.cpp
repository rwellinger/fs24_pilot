/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include <catch2/catch_amalgamated.hpp>

#include <optional>

#include "application/landing_monitor.hpp"
#include "fakes/fake_ports.hpp"
#include "support.hpp"

using namespace fs24;
using domain::FrameInput;
using domain::Grade;

namespace
{

domain::ProfileCatalog catalog()
{
    domain::ProfileCatalog catalog;
    catalog.profiles = {
        // 500 fpm sits clearly inside the HARD band, away from any boundary.
        {"test_ga", {"test_ga", false, {-200, -300, -400, -600}}},
        {"medium_ga", {"medium_ga", false, {-125, -250, -350, -600}}},
        {"turbine_helicopter", {"turbine_helicopter", true, {-75, -150, -300, -500}}},
    };
    catalog.aircraft = {{"C172", "test_ga"}};
    return catalog;
}

// The LSZB RWY 14 geometry; the approach in test::Approach flies heading 140 at 46.9 / 7.5,
// so the touchdown is placed on the field but not necessarily on the runway.
domain::AirportSurfaces lszb()
{
    domain::AirportSurfaces surfaces;
    surfaces.ident   = "LSZB";
    surfaces.runways = {domain::Runway{
        {domain::RunwayEnd{"14", 46.9193252, 7.4908128, 200.f}, domain::RunwayEnd{"32", 46.9073896, 7.5053621, 0.f}},
        30.f}};
    return surfaces;
}

struct Fixture
{
    test::FakeAircraft          aircraft;
    test::FakeRunways           runways;
    test::FakeRepository        repository;
    test::FakePublisher         publisher;
    test::FakeLogger            logger;
    application::LandingMonitor monitor{domain::ProfileResolver(catalog()),
                                        {aircraft, runways, repository, publisher, logger}};

    void step(FrameInput frame, double t)
    {
        frame.sim_time_s = t;
        monitor.on_frame(frame);
    }

    void fly_circuit()
    {
        step(test::parked(), 0.0);
        step(test::takeoff_roll(), 0.05);
        step(test::climbing(), 0.1);
        test::Approach approach(20.0, 100.0);
        auto           sink = [this](const FrameInput &frame)
        {
            monitor.on_frame(frame);
            return std::optional<domain::Landing>{};
        };
        approach.descend_to_touchdown(sink);
        approach.nose_down(sink);
    }
};

} // namespace

TEST_CASE("A landing is rated against the aircraft's profile, stored and published", "[landing_monitor]")
{
    Fixture fixture;
    fixture.monitor.on_aircraft_loaded();
    REQUIRE(fixture.monitor.profile().name == "test_ga");

    fixture.fly_circuit();

    REQUIRE(fixture.repository.stored.size() == 1);
    REQUIRE(fixture.publisher.published.size() == 1);
    const domain::Landing &landing = fixture.publisher.published.front();
    CHECK(landing.profile_name == "test_ga");
    CHECK(landing.descent_fpm == Catch::Approx(-500.f).epsilon(0.02));
    CHECK(landing.grade == Grade::Hard);
    CHECK(fixture.runways.prefetches == 1);
}

TEST_CASE("A landing near a known airport is placed on its surfaces", "[landing_monitor]")
{
    Fixture fixture;
    fixture.runways.surfaces     = lszb();
    fixture.runways.has_surfaces = true;
    fixture.monitor.on_aircraft_loaded();
    fixture.fly_circuit();

    REQUIRE(fixture.publisher.published.size() == 1);
    const domain::Landing &landing = fixture.publisher.published.front();
    CHECK(landing.airport_ident == "LSZB");
    CHECK(landing.distance_to_surface_m >= 0.f);
}

TEST_CASE("Inactive frames (pause, replay, slew) are ignored", "[landing_monitor]")
{
    Fixture fixture;
    fixture.monitor.on_aircraft_loaded();
    FrameInput paused_roll = test::takeoff_roll();
    paused_roll.active     = false;
    fixture.step(paused_roll, 0.0);
    CHECK(fixture.monitor.phases().phase() == domain::FlightPhase::Idle);
}

TEST_CASE("A new aircraft resolves its own profile and starts over", "[landing_monitor]")
{
    Fixture fixture;
    fixture.monitor.on_aircraft_loaded();
    fixture.step(test::takeoff_roll(), 0.0);
    REQUIRE(fixture.monitor.phases().phase() == domain::FlightPhase::Rolling);

    fixture.aircraft.aircraft = {"EC35", true, 3000.f, 2, domain::EngineKind::Turboprop, ""};
    fixture.monitor.on_aircraft_loaded();
    CHECK(fixture.monitor.profile().name == "turbine_helicopter");
    CHECK(fixture.monitor.phases().phase() == domain::FlightPhase::Idle);
}

TEST_CASE("Every transition and the landing summary are logged", "[landing_monitor]")
{
    Fixture fixture;
    fixture.monitor.on_aircraft_loaded();
    fixture.fly_circuit();
    const auto &lines = fixture.logger.lines;
    REQUIRE(lines.size() >= 4);
    CHECK(lines.back().find("landing: HARD LANDING!") == 0);
    CHECK(lines.back().find(", wind 0 kts (head ") != std::string::npos);
}

TEST_CASE("Repeated reports of the same aircraft are logged once", "[landing_monitor]")
{
    Fixture fixture;
    fixture.monitor.on_aircraft_loaded();
    fixture.monitor.on_aircraft_loaded();
    CHECK(fixture.logger.lines.size() == 1);

    fixture.aircraft.aircraft = {"EC35", true, 3000.f, 2, domain::EngineKind::Turboprop, ""};
    fixture.monitor.on_aircraft_loaded();
    CHECK(fixture.logger.lines.size() == 2);
}
