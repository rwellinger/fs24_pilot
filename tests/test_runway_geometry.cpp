/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include <catch2/catch_amalgamated.hpp>

#include <vector>

#include "domain/runway_geometry.hpp"

using namespace fs24::domain;
namespace geo = fs24::domain::runway_geometry;

namespace
{

// LSZB Bern-Belp as in the xp_pilot apt.dat fixture: RWY 14 has a 200 m displaced threshold.
AirportSurfaces lszb()
{
    AirportSurfaces surfaces;
    surfaces.ident   = "LSZB";
    surfaces.runways = {
        Runway{{RunwayEnd{"16", 46.9129619, 7.4946299, 0.f}, RunwayEnd{"34", 46.9085896, 7.4968876, 0.f}}, 30.f},
        Runway{{RunwayEnd{"14", 46.9193252, 7.4908128, 200.f}, RunwayEnd{"32", 46.9073896, 7.5053621, 0.f}}, 30.f},
    };
    surfaces.helipads = {Helipad{46.9108687, 7.5032009}};
    return surfaces;
}

// Derived independently with spherical direct/inverse formulas, measured from the
// displaced threshold of RWY 14 (course 140.21 deg true).
constexpr double TD_ON_CENTERLINE_LAT = 46.9151789;
constexpr double TD_ON_CENTERLINE_LON = 7.4958682;
constexpr double TD_5M_RIGHT_LAT      = 46.9151501;
constexpr double TD_5M_RIGHT_LON      = 7.4958176;
constexpr double RWY14_COURSE_DEG     = 140.21;
constexpr double RWY14_USABLE_M       = 1527.0;

} // namespace

TEST_CASE("locate_touchdown measures distance from the displaced threshold", "[runway]")
{
    const auto fix =
        geo::locate_touchdown(lszb().runways, TD_ON_CENTERLINE_LAT, TD_ON_CENTERLINE_LON, RWY14_COURSE_DEG);
    REQUIRE(fix.has_value());
    CHECK(fix->runway_ident == "14");
    CHECK(fix->distance_from_thr_m == Catch::Approx(400.0).margin(2.0));
    CHECK(fix->centerline_offset_m == Catch::Approx(0.0).margin(1.0));
    CHECK(fix->runway_length_m == Catch::Approx(RWY14_USABLE_M).margin(5.0));
}

TEST_CASE("locate_touchdown signs the centerline offset positive to the right", "[runway]")
{
    const auto right = geo::locate_touchdown(lszb().runways, TD_5M_RIGHT_LAT, TD_5M_RIGHT_LON, RWY14_COURSE_DEG);
    REQUIRE(right.has_value());
    CHECK(right->runway_ident == "14");
    CHECK(right->centerline_offset_m == Catch::Approx(5.0).margin(0.5));

    const auto reverse =
        geo::locate_touchdown(lszb().runways, TD_5M_RIGHT_LAT, TD_5M_RIGHT_LON, RWY14_COURSE_DEG - 180.0);
    REQUIRE(reverse.has_value());
    CHECK(reverse->runway_ident == "32");
    CHECK(reverse->centerline_offset_m == Catch::Approx(-5.0).margin(0.5));
}

TEST_CASE("locate_touchdown picks the landing direction from the heading", "[runway]")
{
    const std::vector<Runway> runways = lszb().runways;
    CHECK(geo::locate_touchdown(runways, TD_ON_CENTERLINE_LAT, TD_ON_CENTERLINE_LON, 140.0)->runway_ident == "14");
    CHECK(geo::locate_touchdown(runways, TD_ON_CENTERLINE_LAT, TD_ON_CENTERLINE_LON, 320.0)->runway_ident == "32");
}

TEST_CASE("locate_touchdown rejects positions and headings off the runway", "[runway]")
{
    const std::vector<Runway> runways = lszb().runways;
    CHECK_FALSE(geo::locate_touchdown(runways, TD_ON_CENTERLINE_LAT, TD_ON_CENTERLINE_LON, 50.0));
    CHECK_FALSE(geo::locate_touchdown(runways, 46.9400000, 7.4908128, RWY14_COURSE_DEG));
    CHECK_FALSE(geo::locate_touchdown(runways, 46.9040000, 7.5100000, RWY14_COURSE_DEG));
    CHECK_FALSE(geo::locate_touchdown({}, TD_ON_CENTERLINE_LAT, TD_ON_CENTERLINE_LON, RWY14_COURSE_DEG));
}

TEST_CASE("distance_to_nearest_surface_m is zero on the runway", "[runway]")
{
    CHECK(geo::distance_to_nearest_surface_m(lszb(), TD_ON_CENTERLINE_LAT, TD_ON_CENTERLINE_LON) ==
          Catch::Approx(0.0).margin(1.0));
    CHECK(geo::distance_to_nearest_surface_m(lszb(), TD_5M_RIGHT_LAT, TD_5M_RIGHT_LON) ==
          Catch::Approx(5.0).margin(0.5));
}

TEST_CASE("distance_to_nearest_surface_m measures to the helipad when it is closest", "[runway]")
{
    AirportSurfaces pad_only;
    pad_only.helipads.push_back({46.9108687, 7.5032009});
    CHECK(geo::distance_to_nearest_surface_m(pad_only, 46.9108687, 7.5032009) == Catch::Approx(0.0).margin(0.1));
    CHECK(geo::distance_to_nearest_surface_m(pad_only, 46.9118687, 7.5032009) == Catch::Approx(111.2).margin(0.5));
}

TEST_CASE("distance_to_nearest_surface_m measures a field beyond the runway end to that end", "[runway]")
{
    CHECK(geo::distance_to_nearest_surface_m(lszb(), 46.9400000, 7.4908128) == Catch::Approx(2299.0).margin(5.0));
}

TEST_CASE("distance_to_nearest_surface_m is unknown for an airport without surfaces", "[runway]")
{
    CHECK(geo::distance_to_nearest_surface_m({}, 46.9, 7.5) == Catch::Approx(-1.0));
}
