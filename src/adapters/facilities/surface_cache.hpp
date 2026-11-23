/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include <algorithm>
#include <cstddef>
#include <deque>
#include <string>
#include <utility>

#include "domain/runway_geometry.hpp"

namespace fs24::adapters::facilities
{

// The airports fetched so far; the one whose surface is nearest to a point answers.
// Airport data does not change during a session, so entries stay until the oldest one
// makes room.
class SurfaceCache
{
  public:
    explicit SurfaceCache(std::size_t capacity) : capacity_(capacity) {}

    bool contains(const std::string &ident) const
    {
        return std::any_of(airports_.begin(), airports_.end(),
                           [&ident](const domain::AirportSurfaces &airport) { return airport.ident == ident; });
    }

    void put(domain::AirportSurfaces surfaces)
    {
        if (contains(surfaces.ident))
        {
            return;
        }
        airports_.push_back(std::move(surfaces));
        while (airports_.size() > capacity_)
        {
            airports_.pop_front();
        }
    }

    // Airports without any runway or helipad cannot be measured against and are skipped.
    const domain::AirportSurfaces *nearest(double latitude_deg, double longitude_deg) const
    {
        const domain::AirportSurfaces *best        = nullptr;
        float                          best_dist_m = 0.f;
        for (const domain::AirportSurfaces &airport : airports_)
        {
            const float dist_m =
                domain::runway_geometry::distance_to_nearest_surface_m(airport, latitude_deg, longitude_deg);
            if (dist_m >= 0.f && (best == nullptr || dist_m < best_dist_m))
            {
                best        = &airport;
                best_dist_m = dist_m;
            }
        }
        return best;
    }

  private:
    std::size_t                         capacity_;
    std::deque<domain::AirportSurfaces> airports_;
};

} // namespace fs24::adapters::facilities
