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

std::string placement(const domain::Landing &landing)
{
    std::array<char, 120> buf{};
    if (landing.runway)
    {
        const domain::RunwayFix &fix = *landing.runway;
        std::snprintf(buf.data(), buf.size(), "%s runway %s, %.0f m past threshold, %+.1f m off centerline",
                      landing.airport_ident.c_str(), fix.runway_ident.c_str(),
                      static_cast<double>(fix.distance_from_thr_m), static_cast<double>(fix.centerline_offset_m));
    }
    else if (landing.off_field)
    {
        std::snprintf(buf.data(), buf.size(), "off-field (%.0f m from %s)",
                      static_cast<double>(landing.distance_to_surface_m),
                      landing.airport_ident.empty() ? "-" : landing.airport_ident.c_str());
    }
    else
    {
        std::snprintf(buf.data(), buf.size(), "%s, no runway",
                      landing.airport_ident.empty() ? "airport unknown" : landing.airport_ident.c_str());
    }
    return buf.data();
}

std::string summary(const domain::Landing &landing)
{
    std::array<char, 360> buf{};
    std::snprintf(buf.data(), buf.size(),
                  "landing: %s %.0f fpm %.2f G, %d bounce(s), wind %.0f kts (head %+.0f, cross %+.0f), profile %s, %s",
                  std::string(domain::label(landing.grade)).c_str(), static_cast<double>(landing.descent_fpm),
                  static_cast<double>(landing.g_force), landing.bounce_count,
                  static_cast<double>(landing.wind_speed_kts), static_cast<double>(landing.wind.headwind_kts),
                  static_cast<double>(landing.wind.crosswind_kts), landing.profile_name.c_str(),
                  placement(landing).c_str());
    return buf.data();
}

bool same_placement(const domain::Landing &a, const domain::Landing &b)
{
    return a.airport_ident == b.airport_ident && a.off_field == b.off_field &&
           a.distance_to_surface_m == b.distance_to_surface_m && a.runway.has_value() == b.runway.has_value();
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
    // Load, flight load and reposition all report the aircraft: log it once per change.
    std::string line = "aircraft " + aircraft_.type_designator + " -> profile " + profile_.name + " (title \"" +
                       aircraft_.title + "\")";
    if (line != last_aircraft_line_)
    {
        ports_.logger.info(line);
        last_aircraft_line_ = std::move(line);
    }
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
    const bool placed = place(landing, false);
    ports_.repository.store(landing);
    ports_.publisher.publish_landing(landing);
    ports_.logger.info(summary(landing));
    unplaced_.reset();
    if (!placed)
    {
        // The prefetch may have missed the field (data still loading, or another airport
        // than the one ahead at 2000 ft): look around the touchdown point itself.
        unplaced_ = std::move(landing);
        ports_.runways.prefetch_near(unplaced_->latitude_deg, unplaced_->longitude_deg);
    }
}

void LandingMonitor::on_runways_arrived()
{
    if (!unplaced_)
    {
        return;
    }
    const bool      lookup_final = !ports_.runways.lookup_pending();
    domain::Landing updated      = *unplaced_;
    const bool      placed       = place(updated, lookup_final);
    if (!same_placement(updated, *unplaced_))
    {
        ports_.repository.replace_latest(updated);
        ports_.publisher.publish_landing(updated);
        ports_.logger.info("placed late: " + placement(updated));
    }
    if (placed || lookup_final)
    {
        unplaced_.reset();
    }
    else
    {
        unplaced_ = std::move(updated);
    }
}

void LandingMonitor::rate(domain::Landing &landing) const
{
    const domain::Rating rating = landing.is_rotorcraft
                                      ? domain::rate_rotorcraft({.descent_fpm    = landing.descent_fpm,
                                                                 .drift_kts      = landing.ground_speed_kts,
                                                                 .bank_deg       = landing.bank_deg,
                                                                 .yaw_rate_deg_s = landing.yaw_rate_deg_s,
                                                                 .g_force        = landing.g_force},
                                                                profile_.thresholds)
                                      : domain::rate_fixed_wing({.descent_fpm   = landing.descent_fpm,
                                                                 .g_force       = landing.g_force,
                                                                 .crosswind_kts = landing.wind.crosswind_kts,
                                                                 .wind          = landing.wind_condition},
                                                                profile_.thresholds);
    landing.grade               = rating.overall;
    landing.criteria            = rating.criteria;
}

// True when the touchdown lies on a known field. Without any airport data the landing
// counts as off-field only once no lookup is outstanding any more.
bool LandingMonitor::place(domain::Landing &landing, bool lookup_final) const
{
    namespace geometry                      = domain::runway_geometry;
    const domain::AirportSurfaces *surfaces = ports_.runways.surfaces_near(landing.latitude_deg, landing.longitude_deg);
    if (surfaces == nullptr)
    {
        landing.off_field = lookup_final;
        return false;
    }
    landing.airport_ident = surfaces->ident;
    landing.distance_to_surface_m =
        geometry::distance_to_nearest_surface_m(*surfaces, landing.latitude_deg, landing.longitude_deg);
    landing.off_field = geometry::is_off_field(landing.distance_to_surface_m);
    landing.runway.reset();
    if (!landing.is_rotorcraft && !landing.off_field)
    {
        landing.runway = geometry::locate_touchdown(surfaces->runways, landing.latitude_deg, landing.longitude_deg,
                                                    landing.heading_true_deg);
    }
    return !landing.off_field;
}

} // namespace fs24::application
