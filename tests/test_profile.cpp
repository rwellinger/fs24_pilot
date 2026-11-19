/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include <catch2/catch_amalgamated.hpp>

#include "domain/profile.hpp"

using namespace fs24::domain;

namespace
{

AircraftIdentity airframe(float mass_kg, int engines, EngineKind kind)
{
    return {"XXXX", false, mass_kg, engines, kind};
}

// Excerpt of the xp_pilot profile list as test data; the FS24 list follows in M06.
ProfileCatalog xp_pilot_catalog()
{
    ProfileCatalog catalog;
    catalog.profiles = {
        {"light_ga", {"light_ga", false, {-100, -200, -300, -500}}},
        {"medium_ga", {"medium_ga", false, {-125, -250, -350, -600}}},
        {"turboprop", {"turboprop", false, {-150, -275, -400, -650}}},
        {"heavy_jet", {"heavy_jet", false, {-250, -400, -600, -850}}},
        {"light_helicopter", {"light_helicopter", true, {-50, -120, -250, -400}}},
        {"turbine_helicopter", {"turbine_helicopter", true, {-75, -150, -300, -500}}},
    };
    catalog.aircraft = {
        {"R22", "light_helicopter"},
        {"C172", "light_ga"},
        {"PC12", "turboprop"},
        {"B738", "heavy_jet"},
    };
    return catalog;
}

} // namespace

TEST_CASE("classify_fixed_wing_profile sorts piston singles by mass", "[profile]")
{
    CHECK(classify_fixed_wing_profile(airframe(450, 1, EngineKind::Piston)) == "ultra_light");
    CHECK(classify_fixed_wing_profile(airframe(1111, 1, EngineKind::Piston)) == "light_ga");
    CHECK(classify_fixed_wing_profile(airframe(1633, 1, EngineKind::Piston)) == "medium_ga");
}

TEST_CASE("classify_fixed_wing_profile treats mass boundaries as inclusive", "[profile]")
{
    CHECK(classify_fixed_wing_profile(airframe(600, 1, EngineKind::Piston)) == "ultra_light");
    CHECK(classify_fixed_wing_profile(airframe(601, 1, EngineKind::Piston)) == "light_ga");
    CHECK(classify_fixed_wing_profile(airframe(1500, 1, EngineKind::Piston)) == "light_ga");
    CHECK(classify_fixed_wing_profile(airframe(1501, 1, EngineKind::Piston)) == "medium_ga");
    CHECK(classify_fixed_wing_profile(airframe(20000, 2, EngineKind::Jet)) == "vlj");
    CHECK(classify_fixed_wing_profile(airframe(20001, 2, EngineKind::Jet)) == "heavy_jet");
}

TEST_CASE("classify_fixed_wing_profile keeps piston twins out of the light profiles", "[profile]")
{
    CHECK(classify_fixed_wing_profile(airframe(1200, 2, EngineKind::Piston)) == "medium_ga");
    CHECK(classify_fixed_wing_profile(airframe(500, 2, EngineKind::Piston)) == "medium_ga");
}

TEST_CASE("classify_fixed_wing_profile rates turboprops as turboprop regardless of mass", "[profile]")
{
    CHECK(classify_fixed_wing_profile(airframe(2200, 1, EngineKind::Turboprop)) == "turboprop");
    CHECK(classify_fixed_wing_profile(airframe(22800, 2, EngineKind::Turboprop)) == "turboprop");
}

TEST_CASE("classify_fixed_wing_profile separates business jets from airliners", "[profile]")
{
    CHECK(classify_fixed_wing_profile(airframe(3921, 1, EngineKind::Jet)) == "vlj");
    CHECK(classify_fixed_wing_profile(airframe(16011, 2, EngineKind::Jet)) == "vlj");
    CHECK(classify_fixed_wing_profile(airframe(38600, 2, EngineKind::Jet)) == "heavy_jet");
    CHECK(classify_fixed_wing_profile(airframe(79000, 2, EngineKind::Jet)) == "heavy_jet");
}

TEST_CASE("classify_fixed_wing_profile declines to guess without a usable mass", "[profile]")
{
    CHECK(classify_fixed_wing_profile(airframe(0, 1, EngineKind::Piston)).empty());
    CHECK(classify_fixed_wing_profile(airframe(-1, 2, EngineKind::Jet)).empty());
}

TEST_CASE("the match list is searched by substring and the first hit wins", "[profile]")
{
    const ProfileResolver resolver(xp_pilot_catalog());
    CHECK(resolver.resolve({"C172", false, 1111.f, 1, EngineKind::Piston}).name == "light_ga");
    CHECK(resolver.resolve({"R22B", true, 0.f, 1, EngineKind::Piston}).name == "light_helicopter");
    CHECK(resolver.resolve({"B738", false, 79000.f, 2, EngineKind::Jet}).thresholds[0] == -250);
}

TEST_CASE("an unlisted fixed-wing aircraft is classified by its airframe", "[profile]")
{
    const ProfileResolver resolver(xp_pilot_catalog());
    CHECK(resolver.resolve({"KODI", false, 3290.f, 1, EngineKind::Turboprop}).name == "turboprop");
    CHECK(resolver.resolve({"A20N", false, 79000.f, 2, EngineKind::Jet}).name == "heavy_jet");
}

TEST_CASE("without list entry or usable airframe data the category fallback applies", "[profile]")
{
    const ProfileResolver resolver(xp_pilot_catalog());
    CHECK(resolver.resolve({"", false, 0.f, 0, EngineKind::Piston}).name == FIXED_WING_FALLBACK_PROFILE);
    CHECK(resolver.resolve({"EC35", true, 3000.f, 2, EngineKind::Turboprop}).name == ROTORCRAFT_FALLBACK_PROFILE);
}

TEST_CASE("a classified profile missing from the catalog falls back instead of rating against nothing", "[profile]")
{
    const ProfileResolver resolver(xp_pilot_catalog()); // has no "vlj"
    const LandingProfile  profile = resolver.resolve({"C510", false, 3921.f, 2, EngineKind::Jet});
    CHECK(profile.name == FIXED_WING_FALLBACK_PROFILE);
    CHECK(profile.thresholds[0] == -125);
}
