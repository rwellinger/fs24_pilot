/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "application/landing_monitor.hpp"

#include <array>
#include <cstdio>
#include <string>
#include <utility>

#include "domain/grade.hpp"
#include "domain/rating_policy.hpp"
#include "domain/runway_geometry.hpp"

namespace fs24::application
{

namespace
{

std::string summary(const domain::Landing &landing)
{
    std::array<char, 160> buf{};
    std::snprintf(buf.data(), buf.size(), "landing: %s %.0f fpm %.2f G, %d bounce(s), profile %s, runway %s",
                  std::string(domain::label(landing.grade)).c_str(), static_cast<double>(landing.descent_fpm),
                  static_cast<double>(landing.g_force), landing.bounce_count, landing.profile_name.c_str(),
                  landing.runway ? landing.runway->runway_ident.c_str() : "-");
    return buf.data();
}

} // namespace

LandingMonitor::LandingMonitor(domain::ProfileResolver resolver, LandingMonitorPorts ports)
    : resolver_(std::move(resolver)), ports_(ports)
{
}

void LandingMonitor::on_aircraft_loaded()
{
    aircraft_ = ports_.aircraft.current_aircraft();
    profile_  = resolver_.resolve(aircraft_);
    phases_.reset();
    runways_prefetched_ = false;
    ports_.logger.info("aircraft " + aircraft_.type_designator + " -> profile " + profile_.name);
}

void LandingMonitor::on_frame(const domain::FrameInput &frame)
{
    if (!frame.active)
    {
        return;
    }
    prefetch_runways(frame);
    domain::PhaseStep step = phases_.step(frame, aircraft_.is_rotorcraft);
    if (step.transition == domain::PhaseTransition::None)
    {
        return;
    }
    ports_.logger.info(domain::transition_label(step.transition));
    if (step.landing)
    {
        finish(std::move(*step.landing));
    }
    if (step.transition != domain::PhaseTransition::Landed)
    {
        runways_prefetched_ = false;
    }
}

void LandingMonitor::prefetch_runways(const domain::FrameInput &frame)
{
    const bool on_approach = phases_.phase() == domain::FlightPhase::Airborne && frame.agl_m < RUNWAY_PREFETCH_AGL_M;
    if (on_approach && !runways_prefetched_)
    {
        ports_.runways.prefetch_near(frame.latitude_deg, frame.longitude_deg);
        runways_prefetched_ = true;
    }
}

void LandingMonitor::finish(domain::Landing landing)
{
    landing.profile_name = profile_.name;
    rate(landing);
    place(landing);
    ports_.repository.store(landing);
    ports_.publisher.publish_landing(landing);
    ports_.logger.info(summary(landing));
}

void LandingMonitor::rate(domain::Landing &landing) const
{
    if (landing.is_rotorcraft)
    {
        landing.grade = domain::rate_rotorcraft({.descent_fpm    = landing.descent_fpm,
                                                 .drift_kts      = landing.ground_speed_kts,
                                                 .bank_deg       = landing.bank_deg,
                                                 .yaw_rate_deg_s = landing.yaw_rate_deg_s,
                                                 .g_force        = landing.g_force},
                                                profile_.thresholds);
        return;
    }
    landing.grade = domain::rate_fixed_wing({.descent_fpm   = landing.descent_fpm,
                                             .g_force       = landing.g_force,
                                             .crosswind_kts = landing.wind.crosswind_kts,
                                             .wind          = landing.wind_condition},
                                            profile_.thresholds);
}

void LandingMonitor::place(domain::Landing &landing) const
{
    const domain::AirportSurfaces *surfaces = ports_.runways.surfaces_near(landing.latitude_deg, landing.longitude_deg);
    if (surfaces == nullptr)
    {
        return;
    }
    landing.airport_ident = surfaces->ident;
    landing.distance_to_surface_m =
        domain::runway_geometry::distance_to_nearest_surface_m(*surfaces, landing.latitude_deg, landing.longitude_deg);
    if (!landing.is_rotorcraft)
    {
        landing.runway = domain::runway_geometry::locate_touchdown(surfaces->runways, landing.latitude_deg,
                                                                   landing.longitude_deg, landing.heading_true_deg);
    }
}

} // namespace fs24::application
