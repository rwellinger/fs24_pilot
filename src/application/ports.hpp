/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include <string_view>

#include "domain/landing.hpp"
#include "domain/profile.hpp"
#include "domain/runway_geometry.hpp"

// What the landing use case needs from the outside world. Implemented by adapters in the
// sim and by fakes in the tests.
namespace fs24::application
{

class AircraftInfoProvider
{
  public:
    virtual ~AircraftInfoProvider()                           = default;
    virtual domain::AircraftIdentity current_aircraft() const = 0;
};

// Runway data is fetched asynchronously by the adapter: prefetch on the approach, read
// what has arrived at touchdown.
class RunwayProvider
{
  public:
    virtual ~RunwayProvider()                                                                             = default;
    virtual void                           prefetch_near(double latitude_deg, double longitude_deg)       = 0;
    virtual const domain::AirportSurfaces *surfaces_near(double latitude_deg, double longitude_deg) const = 0;
};

class LandingRepository
{
  public:
    virtual ~LandingRepository()                       = default;
    virtual void store(const domain::Landing &landing) = 0;
};

class ResultPublisher
{
  public:
    virtual ~ResultPublisher()                                   = default;
    virtual void publish_landing(const domain::Landing &landing) = 0;
};

class Logger
{
  public:
    virtual ~Logger()                           = default;
    virtual void info(std::string_view message) = 0;
};

} // namespace fs24::application
