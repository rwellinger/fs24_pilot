/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include "application/ports.hpp"
#include "domain/flight_phase_machine.hpp"
#include "domain/frame.hpp"
#include "domain/landing.hpp"
#include "domain/profile.hpp"

namespace fs24::application
{

// ~2000 ft AGL leaves roughly a minute for the runways to arrive before touchdown.
inline constexpr float RUNWAY_PREFETCH_AGL_M = 610.f;

struct LandingMonitorPorts
{
    AircraftInfoProvider &aircraft;
    RunwayProvider       &runways;
    LandingRepository    &repository;
    ResultPublisher      &publisher;
    Logger               &logger;
};

// Use case: turn the stream of sim frames into rated, placed, stored and published landings.
class LandingMonitor
{
  public:
    LandingMonitor(domain::ProfileResolver resolver, LandingMonitorPorts ports);

    // A new aircraft or a reposition: resolve its profile and start over.
    void on_aircraft_loaded();

    void on_frame(const domain::FrameInput &frame);

    const domain::LandingProfile     &profile() const { return profile_; }
    const domain::FlightPhaseMachine &phases() const { return phases_; }

  private:
    void prefetch_runways(const domain::FrameInput &frame);
    void finish(domain::Landing landing);
    void rate(domain::Landing &landing) const;
    void place(domain::Landing &landing) const;

    domain::ProfileResolver    resolver_;
    LandingMonitorPorts        ports_;
    domain::AircraftIdentity   aircraft_;
    domain::LandingProfile     profile_;
    domain::FlightPhaseMachine phases_;
    bool                       runways_prefetched_ = false;
};

} // namespace fs24::application
