/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include "application/ports.hpp"

namespace fs24::adapters::facilities
{

// Until the Facilities API adapter lands (M05): landings are rated but not placed.
class NoRunwayData final : public application::RunwayProvider
{
  public:
    void prefetch_near(double /*latitude_deg*/, double /*longitude_deg*/) override {}

    const domain::AirportSurfaces *surfaces_near(double /*latitude_deg*/, double /*longitude_deg*/) const override
    {
        return nullptr;
    }
};

} // namespace fs24::adapters::facilities
