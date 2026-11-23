/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "adapters/facilities/facility_mapping.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <string>
#include <utility>
#include <vector>

namespace fs24::adapters::facilities
{

namespace
{

constexpr double EARTH_RADIUS_M = 6371008.8; // same mean radius as domain::runway_geometry
constexpr double DEG_TO_RAD     = std::numbers::pi / 180.0;
constexpr double RAD_TO_DEG     = 180.0 / std::numbers::pi;

constexpr int LAST_NUMBERED_RUNWAY = 36;

constexpr std::array<const char *, 8> COMPASS_IDENTS{"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
constexpr std::array<const char *, 7> DESIGNATOR_SUFFIXES{"", "L", "R", "C", "W", "A", "B"};
constexpr int                         COMPASS_COUNT    = static_cast<int>(COMPASS_IDENTS.size());
constexpr int                         DESIGNATOR_COUNT = static_cast<int>(DESIGNATOR_SUFFIXES.size());

struct LatLon
{
    double latitude_deg  = 0;
    double longitude_deg = 0;
};

// Moves a point along a true course; flat-earth, fine for half a runway length.
LatLon offset(LatLon from, double course_deg, double distance_m)
{
    const double course = course_deg * DEG_TO_RAD;
    const double north  = std::cos(course) * distance_m;
    const double east   = std::sin(course) * distance_m;
    return {.latitude_deg = from.latitude_deg + north / EARTH_RADIUS_M * RAD_TO_DEG,
            .longitude_deg =
                from.longitude_deg + east / (EARTH_RADIUS_M * std::cos(from.latitude_deg * DEG_TO_RAD)) * RAD_TO_DEG};
}

float threshold_length(const RawPavement &raw) { return raw.enabled != 0 ? raw.length_m : 0.f; }

} // namespace

std::string to_ident(const char *chars, std::size_t capacity)
{
    std::size_t length = 0;
    while (length < capacity && chars[length] != '\0')
    {
        ++length;
    }
    return {chars, length};
}

std::string runway_designator(int number, int designator)
{
    std::string ident;
    if (number >= 1 && number <= LAST_NUMBERED_RUNWAY)
    {
        std::array<char, 4> digits{};
        std::snprintf(digits.data(), digits.size(), "%02d", number);
        ident = digits.data();
    }
    else if (number > LAST_NUMBERED_RUNWAY && number - LAST_NUMBERED_RUNWAY <= COMPASS_COUNT)
    {
        ident = COMPASS_IDENTS[static_cast<std::size_t>(number - LAST_NUMBERED_RUNWAY - 1)];
    }
    if (designator > 0 && designator < DESIGNATOR_COUNT)
    {
        ident += DESIGNATOR_SUFFIXES[static_cast<std::size_t>(designator)];
    }
    return ident;
}

// The primary end is the one landed on when flying the runway heading, so it lies half a
// length behind the centre.
domain::Runway to_runway(const RawRunway &raw, float primary_threshold_m, float secondary_threshold_m)
{
    const LatLon centre{.latitude_deg = raw.latitude_deg, .longitude_deg = raw.longitude_deg};
    const double half_length_m = raw.length_m * 0.5;
    const LatLon primary       = offset(centre, raw.heading_deg + 180.0, half_length_m);
    const LatLon secondary     = offset(centre, raw.heading_deg, half_length_m);
    return domain::Runway{
        .ends    = {domain::RunwayEnd{.designator    = runway_designator(raw.primary_number, raw.primary_designator),
                                      .latitude_deg  = primary.latitude_deg,
                                      .longitude_deg = primary.longitude_deg,
                                      .displaced_threshold_m = primary_threshold_m},
                    domain::RunwayEnd{.designator    = runway_designator(raw.secondary_number, raw.secondary_designator),
                                      .latitude_deg  = secondary.latitude_deg,
                                      .longitude_deg = secondary.longitude_deg,
                                      .displaced_threshold_m = secondary_threshold_m}},
        .width_m = raw.width_m};
}

std::vector<std::string> nearby_idents(std::vector<ListedAirport> airports, double latitude_deg, double longitude_deg)
{
    const auto distance_m = [latitude_deg, longitude_deg](const ListedAirport &airport)
    {
        return domain::runway_geometry::distance_m(latitude_deg, longitude_deg, airport.latitude_deg,
                                                   airport.longitude_deg);
    };
    std::sort(airports.begin(), airports.end(),
              [&distance_m](const ListedAirport &a, const ListedAirport &b) { return distance_m(a) < distance_m(b); });
    std::vector<std::string> idents;
    for (const ListedAirport &airport : airports)
    {
        if (idents.size() == NEARBY_AIRPORTS_MAX || distance_m(airport) > NEARBY_AIRPORT_RADIUS_M)
        {
            break;
        }
        idents.push_back(airport.ident);
    }
    return idents;
}

void AirportAssembler::set_airport(const RawAirport &raw) { airport_ = raw; }

void AirportAssembler::add_runway(std::uint32_t unique_id, const RawRunway &raw)
{
    runways_.push_back({.unique_id = unique_id, .raw = raw, .thresholds_m = {}});
}

void AirportAssembler::add_threshold(std::uint32_t parent_id, const RawPavement &raw)
{
    for (PendingRunway &runway : runways_)
    {
        if (runway.unique_id == parent_id)
        {
            runway.thresholds_m.push_back(threshold_length(raw));
            return;
        }
    }
}

void AirportAssembler::add_helipad(const RawHelipad &raw) { helipads_.push_back(raw); }

domain::AirportSurfaces AirportAssembler::finish() const
{
    domain::AirportSurfaces surfaces;
    if (airport_)
    {
        surfaces.ident = to_ident(airport_->icao.data(), airport_->icao.size());
    }
    for (const PendingRunway &runway : runways_)
    {
        const auto threshold = [&runway](std::size_t end)
        { return end < runway.thresholds_m.size() ? runway.thresholds_m[end] : 0.f; };
        surfaces.runways.push_back(to_runway(runway.raw, threshold(0), threshold(1)));
    }
    for (const RawHelipad &helipad : helipads_)
    {
        surfaces.helipads.push_back({.latitude_deg = helipad.latitude_deg, .longitude_deg = helipad.longitude_deg});
    }
    return surfaces;
}

std::string AirportAssembler::describe() const
{
    const domain::AirportSurfaces surfaces = finish();
    std::array<char, 64>          buf{};
    std::snprintf(buf.data(), buf.size(), "runways %s (magvar %.1f):", surfaces.ident.c_str(),
                  airport_ ? static_cast<double>(airport_->magvar_deg) : 0.0);
    std::string line = buf.data();
    for (std::size_t i = 0; i < runways_.size(); ++i)
    {
        const RawRunway      &raw = runways_[i].raw;
        const domain::Runway &rwy = surfaces.runways[i];
        std::array<char, 96>  part{};
        std::snprintf(part.data(), part.size(), " %s/%s hdg %.1f len %.0f thr %.0f/%.0f;",
                      rwy.ends[0].designator.c_str(), rwy.ends[1].designator.c_str(),
                      static_cast<double>(raw.heading_deg), static_cast<double>(raw.length_m),
                      static_cast<double>(rwy.ends[0].displaced_threshold_m),
                      static_cast<double>(rwy.ends[1].displaced_threshold_m));
        line += part.data();
    }
    return line + " " + std::to_string(helipads_.size()) + " helipad(s)";
}

} // namespace fs24::adapters::facilities
