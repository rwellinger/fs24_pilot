/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include <catch2/catch_amalgamated.hpp>

#include <array>
#include <cstddef>
#include <cstring>

#include "adapters/facilities/facility_mapping.hpp"
#include "adapters/facilities/surface_cache.hpp"
#include "domain/runway_geometry.hpp"

using namespace fs24::adapters::facilities;
namespace geometry = fs24::domain::runway_geometry;

namespace
{

// Bern-Belp RWY 14/32 by its centre, true heading 138, 1730 m with a 200 m displaced 14.
RawRunway lszb_raw()
{
    return RawRunway{.latitude_deg         = 46.9133574,
                     .longitude_deg        = 7.4980875,
                     .heading_deg          = 138.f,
                     .length_m             = 1730.f,
                     .width_m              = 30.f,
                     .primary_number       = 14,
                     .primary_designator   = 0,
                     .secondary_number     = 32,
                     .secondary_designator = 0};
}

RawAirport airport_raw(const char *icao)
{
    RawAirport raw{};
    std::strncpy(raw.icao.data(), icao, raw.icao.size());
    return raw;
}

fs24::domain::AirportSurfaces airport_at(const char *ident, double latitude_deg, double longitude_deg)
{
    RawRunway raw     = lszb_raw();
    raw.latitude_deg  = latitude_deg;
    raw.longitude_deg = longitude_deg;
    fs24::domain::AirportSurfaces surfaces;
    surfaces.ident   = ident;
    surfaces.runways = {to_runway(raw, 0.f, 0.f)};
    return surfaces;
}

} // namespace

TEST_CASE("Runway idents are built from the number and designator codes", "[facilities]")
{
    CHECK(runway_designator(9, 0) == "09");
    CHECK(runway_designator(27, 1) == "27L");
    CHECK(runway_designator(18, 2) == "18R");
    CHECK(runway_designator(1, 3) == "01C");
    CHECK(runway_designator(36, 4) == "36W");
    CHECK(runway_designator(37, 0) == "N");
    CHECK(runway_designator(44, 0) == "NW");
    CHECK(runway_designator(0, 0).empty());
    CHECK(runway_designator(14, 7) == "14"); // LAST is no designator
}

TEST_CASE("The runway ends lie half a length either side of the centre, primary behind it", "[facilities]")
{
    const fs24::domain::Runway runway = to_runway(lszb_raw(), 200.f, 0.f);
    const auto &[primary, secondary]  = runway.ends;
    CHECK(primary.designator == "14");
    CHECK(secondary.designator == "32");
    CHECK(primary.displaced_threshold_m == 200.f);
    CHECK(geometry::distance_m(primary.latitude_deg, primary.longitude_deg, secondary.latitude_deg,
                               secondary.longitude_deg) == Catch::Approx(1730.f).epsilon(0.001));
    CHECK(primary.latitude_deg > secondary.latitude_deg); // 14 is the north-west end
    CHECK(primary.longitude_deg < secondary.longitude_deg);
}

TEST_CASE("A touchdown on the converted runway is measured from the displaced threshold", "[facilities]")
{
    const fs24::domain::Runway runway = to_runway(lszb_raw(), 200.f, 0.f);
    const RawRunway            raw    = lszb_raw();
    // The centre is 865 m past the pavement start, 665 m past the displaced threshold.
    const auto fix = geometry::locate_touchdown({runway}, raw.latitude_deg, raw.longitude_deg, 138.0);
    REQUIRE(fix.has_value());
    CHECK(fix->runway_ident == "14");
    CHECK(fix->distance_from_thr_m == Catch::Approx(665.f).margin(1.f));
    CHECK(fix->centerline_offset_m == Catch::Approx(0.f).margin(0.5f));
    CHECK(fix->runway_length_m == Catch::Approx(1530.f).margin(1.f));
}

TEST_CASE("Facility blocks are read tightly packed and refused when too short", "[facilities]")
{
    static_assert(sizeof(RawAirport) == 28);
    static_assert(sizeof(RawRunway) == 44);
    static_assert(sizeof(RawPavement) == 8);
    static_assert(sizeof(RawHelipad) == 16);

    std::array<std::byte, sizeof(RawPavement)> bytes{};
    const float                                length_m = 150.f;
    const std::int32_t                         enabled  = 1;
    std::memcpy(bytes.data(), &length_m, sizeof(length_m));
    std::memcpy(bytes.data() + sizeof(length_m), &enabled, sizeof(enabled));

    RawPavement pavement{};
    REQUIRE(read_block(bytes.data(), bytes.size(), pavement));
    CHECK(pavement.length_m == 150.f);
    CHECK(pavement.enabled == 1);
    CHECK_FALSE(read_block(bytes.data(), bytes.size() - 1, pavement));
}

TEST_CASE("An ident that fills its field is read without a terminator", "[facilities]")
{
    const std::array<char, 8> full{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
    CHECK(to_ident(full.data(), full.size()) == "ABCDEFGH");
    const std::array<char, 8> short_ident{'L', 'S', 'Z', 'B'};
    CHECK(to_ident(short_ident.data(), short_ident.size()) == "LSZB");
}

TEST_CASE("Only the nearest airports within reach are fetched", "[facilities]")
{
    std::vector<ListedAirport> listed{
        {.ident = "FAR", .latitude_deg = 47.2, .longitude_deg = 7.5},   // ~33 km
        {.ident = "NEAR", .latitude_deg = 46.91, .longitude_deg = 7.5}, // ~1 km
        {.ident = "MID", .latitude_deg = 46.95, .longitude_deg = 7.5},  // ~5.6 km
    };
    CHECK(nearby_idents(listed, 46.9, 7.5) == std::vector<std::string>{"NEAR", "MID"});

    std::vector<ListedAirport> crowded;
    for (int i = 0; i < 10; ++i)
    {
        crowded.push_back({.ident = std::to_string(i), .latitude_deg = 46.9 + 0.001 * i, .longitude_deg = 7.5});
    }
    const auto idents = nearby_idents(crowded, 46.9, 7.5);
    REQUIRE(idents.size() == NEARBY_AIRPORTS_MAX);
    CHECK(idents.front() == "0");
}

TEST_CASE("An airport is assembled from its messages, thresholds matched by parent", "[facilities]")
{
    AirportAssembler assembler;
    assembler.set_airport(airport_raw("LSZB"));
    assembler.add_runway(11, lszb_raw());
    RawRunway grass          = lszb_raw();
    grass.primary_number     = 14;
    grass.primary_designator = 1;
    assembler.add_runway(12, grass);
    assembler.add_threshold(11, {.length_m = 200.f, .enabled = 1});
    assembler.add_threshold(12, {.length_m = 90.f, .enabled = 0}); // disabled: no displacement
    assembler.add_threshold(11, {.length_m = 0.f, .enabled = 1});
    assembler.add_helipad({.latitude_deg = 46.91, .longitude_deg = 7.50});

    const fs24::domain::AirportSurfaces surfaces = assembler.finish();
    CHECK(surfaces.ident == "LSZB");
    REQUIRE(surfaces.runways.size() == 2);
    CHECK(surfaces.runways[0].ends[0].displaced_threshold_m == 200.f);
    CHECK(surfaces.runways[0].ends[1].displaced_threshold_m == 0.f);
    CHECK(surfaces.runways[1].ends[0].designator == "14L");
    CHECK(surfaces.runways[1].ends[0].displaced_threshold_m == 0.f);
    CHECK(surfaces.helipads.size() == 1);
    CHECK(assembler.describe().find("runways LSZB (magvar 0.0): 14/32 hdg 138.0 len 1730 thr 200/0;") == 0);
}

TEST_CASE("The cache answers with the airport whose surface is nearest", "[facilities]")
{
    SurfaceCache cache(2);
    CHECK(cache.nearest(46.9, 7.5) == nullptr);

    fs24::domain::AirportSurfaces seaplane_base;
    seaplane_base.ident = "EMPTY";
    cache.put(seaplane_base);
    CHECK(cache.nearest(46.9, 7.5) == nullptr);

    cache.put(airport_at("NEAR", 46.9, 7.5));
    cache.put(airport_at("FAR", 46.95, 7.5)); // evicts the oldest
    CHECK_FALSE(cache.contains("EMPTY"));
    CHECK(cache.contains("FAR"));
    REQUIRE(cache.nearest(46.9, 7.5) != nullptr);
    CHECK(cache.nearest(46.9, 7.5)->ident == "NEAR");
    CHECK(cache.nearest(46.96, 7.5)->ident == "FAR");
}
