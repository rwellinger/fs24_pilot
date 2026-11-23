/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "domain/runway_fix.hpp"

namespace fs24::domain
{

struct RunwayEnd
{
    std::string designator;
    double      latitude_deg          = 0;
    double      longitude_deg         = 0;
    float       displaced_threshold_m = 0;
};

struct Runway
{
    std::array<RunwayEnd, 2> ends;
    float                    width_m = 0;
};

struct Helipad
{
    double latitude_deg  = 0;
    double longitude_deg = 0;
};

struct AirportSurfaces
{
    std::string          ident;
    std::vector<Runway>  runways;
    std::vector<Helipad> helipads;
};

namespace runway_geometry
{

// Farther than this from every runway and helipad of the nearest airport is off-field.
inline constexpr float OFF_FIELD_DISTANCE_M = 500.f;

// `distance_to_surface_m` as returned by distance_to_nearest_surface_m; an airport without
// surfaces counts as off-field.
inline bool is_off_field(float distance_to_surface_m)
{
    return distance_to_surface_m < 0.f || distance_to_surface_m > OFF_FIELD_DISTANCE_M;
}

// Ground distance; equirectangular, meant for the area around an airport.
float distance_m(double lat_a, double lon_a, double lat_b, double lon_b);

// Distance to the nearest runway centerline or helipad, -1 when the airport has neither.
float distance_to_nearest_surface_m(const AirportSurfaces &surfaces, double lat, double lon);

// The runway direction the aircraft touched down on; among the candidates that fit
// heading and pavement the one closest to the centerline wins.
std::optional<RunwayFix> locate_touchdown(const std::vector<Runway> &runways, double lat, double lon,
                                          double heading_true_deg);

} // namespace runway_geometry

} // namespace fs24::domain
