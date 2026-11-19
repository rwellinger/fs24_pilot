/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "domain/runway_geometry.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <optional>
#include <vector>

namespace fs24::domain::runway_geometry
{

namespace
{

// Mean radius: the equatorial one overstates runway lengths by ~0.1% at mid latitudes.
constexpr double EARTH_RADIUS_M = 6371008.8;
constexpr double DEG_TO_RAD     = std::numbers::pi / 180.0;
constexpr double RAD_TO_DEG     = 180.0 / std::numbers::pi;

// Beyond these a landing belongs to another runway, or to none.
constexpr double MAX_HEADING_DIFF_DEG = 45.0;
constexpr double MAX_OFFSET_M         = 75.0;
constexpr double MAX_UNDERSHOOT_M     = 100.0;
constexpr double MIN_AXIS_LENGTH_M    = 1.0;

struct LocalPoint
{
    double east_m  = 0;
    double north_m = 0;
};

// Equirectangular projection; at runway scale the error is centimetres.
LocalPoint to_local_meters(double lat_ref, double lon_ref, double lat, double lon)
{
    const double mean_lat = (lat_ref + lat) * 0.5 * DEG_TO_RAD;
    return {.east_m  = (lon - lon_ref) * DEG_TO_RAD * EARTH_RADIUS_M * std::cos(mean_lat),
            .north_m = (lat - lat_ref) * DEG_TO_RAD * EARTH_RADIUS_M};
}

double normalize_deg(double deg)
{
    const double wrapped = std::fmod(deg, 360.0);
    return wrapped < 0.0 ? wrapped + 360.0 : wrapped;
}

double heading_difference_deg(double a, double b)
{
    const double diff = std::fabs(normalize_deg(a) - normalize_deg(b));
    return diff > 180.0 ? 360.0 - diff : diff;
}

std::optional<RunwayFix> evaluate(const Runway &runway, std::size_t landing_end, double td_lat, double td_lon,
                                  double heading_true_deg)
{
    const RunwayEnd &near_end = runway.ends[landing_end];
    const RunwayEnd &far_end  = runway.ends[1 - landing_end];

    const LocalPoint far_local =
        to_local_meters(near_end.latitude_deg, near_end.longitude_deg, far_end.latitude_deg, far_end.longitude_deg);
    const double axis_len = std::hypot(far_local.east_m, far_local.north_m);
    if (axis_len < MIN_AXIS_LENGTH_M)
    {
        return std::nullopt;
    }
    const double forward_east  = far_local.east_m / axis_len;
    const double forward_north = far_local.north_m / axis_len;

    const double course_deg = normalize_deg(std::atan2(forward_east, forward_north) * RAD_TO_DEG);
    if (heading_difference_deg(course_deg, heading_true_deg) > MAX_HEADING_DIFF_DEG)
    {
        return std::nullopt;
    }

    // The usable landing surface starts at the displaced threshold, not at the pavement edge.
    const LocalPoint td        = to_local_meters(near_end.latitude_deg, near_end.longitude_deg, td_lat, td_lon);
    const double     rel_east  = td.east_m - forward_east * near_end.displaced_threshold_m;
    const double     rel_north = td.north_m - forward_north * near_end.displaced_threshold_m;

    const double along    = rel_east * forward_east + rel_north * forward_north;
    const double offset   = rel_east * forward_north - rel_north * forward_east; // right of travel = +
    const double length_m = axis_len - near_end.displaced_threshold_m;

    if (std::fabs(offset) > MAX_OFFSET_M || along < -MAX_UNDERSHOOT_M || along > length_m)
    {
        return std::nullopt;
    }
    return RunwayFix{.runway_ident        = near_end.designator,
                     .centerline_offset_m = static_cast<float>(offset),
                     .distance_from_thr_m = static_cast<float>(along),
                     .runway_length_m     = static_cast<float>(length_m)};
}

double distance_to_segment_m(const RunwayEnd &a, const RunwayEnd &b, double lat, double lon)
{
    const LocalPoint end   = to_local_meters(a.latitude_deg, a.longitude_deg, b.latitude_deg, b.longitude_deg);
    const LocalPoint point = to_local_meters(a.latitude_deg, a.longitude_deg, lat, lon);
    const double     len2  = end.east_m * end.east_m + end.north_m * end.north_m;
    const double     t =
        len2 > 0.0 ? std::clamp((point.east_m * end.east_m + point.north_m * end.north_m) / len2, 0.0, 1.0) : 0.0;
    return std::hypot(point.east_m - t * end.east_m, point.north_m - t * end.north_m);
}

} // namespace

float distance_m(double lat_a, double lon_a, double lat_b, double lon_b)
{
    const LocalPoint p = to_local_meters(lat_a, lon_a, lat_b, lon_b);
    return static_cast<float>(std::hypot(p.east_m, p.north_m));
}

float distance_to_nearest_surface_m(const AirportSurfaces &surfaces, double lat, double lon)
{
    double nearest = -1.0;
    auto   keep    = [&nearest](double d) { nearest = nearest < 0.0 ? d : std::min(nearest, d); };
    for (const Runway &runway : surfaces.runways)
    {
        keep(distance_to_segment_m(runway.ends[0], runway.ends[1], lat, lon));
    }
    for (const Helipad &helipad : surfaces.helipads)
    {
        keep(distance_m(helipad.latitude_deg, helipad.longitude_deg, lat, lon));
    }
    return static_cast<float>(nearest);
}

std::optional<RunwayFix> locate_touchdown(const std::vector<Runway> &runways, double lat, double lon,
                                          double heading_true_deg)
{
    std::optional<RunwayFix> best;
    for (const Runway &runway : runways)
    {
        for (std::size_t end = 0; end < runway.ends.size(); ++end)
        {
            const auto candidate = evaluate(runway, end, lat, lon, heading_true_deg);
            if (candidate &&
                (!best || std::fabs(candidate->centerline_offset_m) < std::fabs(best->centerline_offset_m)))
            {
                best = candidate;
            }
        }
    }
    return best;
}

} // namespace fs24::domain::runway_geometry
