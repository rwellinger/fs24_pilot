/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "application/ports.hpp"

namespace fs24::test
{

class FakeAircraft final : public application::AircraftInfoProvider
{
  public:
    domain::AircraftIdentity current_aircraft() const override { return aircraft; }

    domain::AircraftIdentity aircraft{"C172", false, 1111.f, 1, domain::EngineKind::Piston, ""};
};

class FakeRunways final : public application::RunwayProvider
{
  public:
    void prefetch_near(double latitude_deg, double /*longitude_deg*/) override
    {
        ++prefetches;
        last_prefetch_latitude_deg = latitude_deg;
    }

    const domain::AirportSurfaces *surfaces_near(double /*latitude_deg*/, double /*longitude_deg*/) const override
    {
        return has_surfaces ? &surfaces : nullptr;
    }

    bool lookup_pending() const override { return pending; }

    domain::AirportSurfaces surfaces;
    bool                    has_surfaces               = false;
    bool                    pending                    = false;
    int                     prefetches                 = 0;
    double                  last_prefetch_latitude_deg = 0;
};

class FakeRepository final : public application::LandingRepository
{
  public:
    void store(const domain::Landing &landing) override { stored.push_back(landing); }

    void replace_latest(const domain::Landing &landing) override
    {
        stored.back() = landing;
        ++replacements;
    }

    std::vector<domain::Landing> stored;
    int                          replacements = 0;
};

class FakePublisher final : public application::ResultPublisher
{
  public:
    void publish_landing(const domain::Landing &landing) override { published.push_back(landing); }

    std::vector<domain::Landing> published;
};

class FakeLogger final : public application::Logger
{
  public:
    void info(std::string_view message) override { lines.emplace_back(message); }

    std::vector<std::string> lines;
};

} // namespace fs24::test
