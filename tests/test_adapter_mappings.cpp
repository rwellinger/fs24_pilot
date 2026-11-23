/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include <catch2/catch_amalgamated.hpp>

#include <cstring>

#include "adapters/profiles/builtin_catalog.hpp"
#include "adapters/simvars/aircraft_mapping.hpp"
#include "adapters/simvars/frame_mapping.hpp"
#include "adapters/storage/memory_landing_repository.hpp"

using namespace fs24::adapters;
using fs24::domain::EngineKind;

namespace
{

simvars::RawFrame on_the_ground()
{
    simvars::RawFrame raw{};
    raw.sim_on_ground         = 1;
    raw.gear_center_on_ground = 1;
    raw.gear_left_on_ground   = 1;
    raw.gear_right_on_ground  = 1;
    return raw;
}

simvars::SimState running() { return {.sim_time_s = 42.0, .running = true, .paused = false}; }

simvars::RawAircraft aircraft(const char *atc_model, const char *category, int engine_type, double mass_lbs,
                              int engines)
{
    simvars::RawAircraft raw{};
    raw.engine_type          = engine_type;
    raw.engine_count         = engines;
    raw.max_gross_weight_lbs = mass_lbs;
    std::strncpy(raw.atc_model.data(), atc_model, raw.atc_model.size() - 1);
    std::strncpy(raw.category.data(), category, raw.category.size() - 1);
    return raw;
}

} // namespace

TEST_CASE("Mains first, nose last: any and all gear follow the individual legs", "[simvars]")
{
    simvars::RawFrame raw     = on_the_ground();
    raw.gear_center_on_ground = 0;
    auto frame                = simvars::to_frame(raw, running(), false);
    CHECK(frame.on_any_gear);
    CHECK_FALSE(frame.on_all_gear);

    frame = simvars::to_frame(on_the_ground(), running(), false);
    CHECK(frame.on_all_gear);

    frame = simvars::to_frame(simvars::RawFrame{}, running(), false);
    CHECK_FALSE(frame.on_any_gear);
}

TEST_CASE("Skids count as down on both skids, or on ground contact when the sim maps none", "[simvars]")
{
    simvars::RawFrame one_skid{};
    one_skid.sim_on_ground        = 1;
    one_skid.skid_front_on_ground = 1;
    CHECK_FALSE(simvars::to_frame(one_skid, running(), true).on_all_gear);

    one_skid.skid_rear_on_ground = 1;
    CHECK(simvars::to_frame(one_skid, running(), true).on_all_gear);

    simvars::RawFrame unmapped{};
    unmapped.sim_on_ground = 1;
    CHECK(simvars::to_frame(unmapped, running(), true).on_all_gear);
}

TEST_CASE("Frames are inactive outside a running, unpaused, unslewed sim", "[simvars]")
{
    CHECK(simvars::to_frame(on_the_ground(), running(), false).active);

    simvars::SimState paused = running();
    paused.paused            = true;
    CHECK_FALSE(simvars::to_frame(on_the_ground(), paused, false).active);

    simvars::SimState menu = running();
    menu.running           = false;
    CHECK_FALSE(simvars::to_frame(on_the_ground(), menu, false).active);

    simvars::RawFrame slewing = on_the_ground();
    slewing.slew_active       = 1;
    CHECK_FALSE(simvars::to_frame(slewing, running(), false).active);
}

TEST_CASE("Pitch is turned nose-up positive and the sim time is passed through", "[simvars]")
{
    simvars::RawFrame raw = on_the_ground();
    raw.pitch_deg         = -5.f; // MSFS: negative = nose up
    raw.pitch_rate_deg_s  = 1.f;
    const auto frame      = simvars::to_frame(raw, running(), false);
    CHECK(frame.pitch_deg == Catch::Approx(5.f));
    CHECK(frame.pitch_rate_deg_s == Catch::Approx(-1.f));
    CHECK(frame.sim_time_s == Catch::Approx(42.0));
}

TEST_CASE("The sim's touchdown velocity becomes a negative descent rate in fpm", "[simvars]")
{
    simvars::RawFrame raw             = on_the_ground();
    raw.touchdown_normal_velocity_fps = 3.43; // MSFS: downward positive
    CHECK(simvars::to_frame(raw, running(), false).sim_touchdown_descent_fpm == Catch::Approx(-205.8f));
}

TEST_CASE("ATC MODEL yields the type designator from a localisation key or plain text", "[simvars]")
{
    CHECK(simvars::type_designator_from_atc_model("TT:ATCCOM.AC_MODEL_C172.0.text") == "C172");
    CHECK(simvars::type_designator_from_atc_model("TT:ATCCOM.AC_MODEL A20N.0.text") == "A20N");
    CHECK(simvars::type_designator_from_atc_model("kodi") == "KODI");
    CHECK(simvars::type_designator_from_atc_model("  EC45 ") == "EC45");
    CHECK(simvars::type_designator_from_atc_model("").empty());
}

TEST_CASE("The aircraft identity maps category, engine type and mass", "[simvars]")
{
    const auto c172 = simvars::to_identity(aircraft("TT:ATCCOM.AC_MODEL_C172.0.text", "Airplane", 0, 2550, 1));
    CHECK(c172.type_designator == "C172");
    CHECK_FALSE(c172.is_rotorcraft);
    CHECK(c172.engine_kind == EngineKind::Piston);
    CHECK(c172.max_takeoff_mass_kg == Catch::Approx(1156.7f).margin(0.5f));

    const auto h145 = simvars::to_identity(aircraft("EC45", "Helicopter", 3, 8157, 2));
    CHECK(h145.is_rotorcraft);

    CHECK(simvars::to_identity(aircraft("KODI", "Airplane", 5, 7255, 1)).engine_kind == EngineKind::Turboprop);
    CHECK(simvars::to_identity(aircraft("A20N", "airplane", 1, 174165, 2)).engine_kind == EngineKind::Jet);

    simvars::RawAircraft tbm = aircraft("850", "Airplane", 5, 7394, 1);
    std::strncpy(tbm.title.data(), "Daher TBM 850", tbm.title.size() - 1);
    CHECK(simvars::to_identity(tbm).title == "Daher TBM 850");
}

TEST_CASE("The built-in catalog classifies the fleet until the FS24 list exists", "[profiles]")
{
    const fs24::domain::ProfileResolver resolver(profiles::builtin_catalog());
    CHECK(resolver.resolve(simvars::to_identity(aircraft("C172", "Airplane", 0, 2550, 1))).name == "light_ga");
    CHECK(resolver.resolve(simvars::to_identity(aircraft("KODI", "Airplane", 5, 7255, 1))).name == "turboprop");
    CHECK(resolver.resolve(simvars::to_identity(aircraft("A20N", "Airplane", 1, 174165, 2))).name == "heavy_jet");
    CHECK(resolver.resolve(simvars::to_identity(aircraft("EC45", "Helicopter", 3, 8157, 2))).name ==
          "turbine_helicopter");
}

TEST_CASE("The memory repository keeps the newest landings up to its capacity", "[storage]")
{
    storage::MemoryLandingRepository repository(2);
    CHECK_FALSE(repository.latest().has_value());
    for (int i = 1; i <= 3; ++i)
    {
        fs24::domain::Landing landing;
        landing.bounce_count = i;
        repository.store(landing);
    }
    REQUIRE(repository.latest().has_value());
    CHECK(repository.latest()->bounce_count == 3);

    fs24::domain::Landing placed_late;
    placed_late.bounce_count = 30;
    repository.replace_latest(placed_late);
    CHECK(repository.latest()->bounce_count == 30);
}

TEST_CASE("Gear is down when extended or when a wheel touches the runway", "[simvars]")
{
    simvars::RawFrame raw   = on_the_ground();
    raw.gear_extended_ratio = 0.0; // add-on gear system that never reports extension
    CHECK(simvars::to_frame(raw, running(), false).gear_down);

    simvars::RawFrame belly{};
    belly.sim_on_ground = 1;
    CHECK_FALSE(simvars::to_frame(belly, running(), false).gear_down);

    belly.gear_extended_ratio = 1.0;
    CHECK(simvars::to_frame(belly, running(), false).gear_down);
}
