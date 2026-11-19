/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "domain/profile.hpp"

#include <string>
#include <string_view>
#include <utility>

namespace fs24::domain
{

namespace
{

// 600 kg is the international light-sport limit; 1500 kg separates the C172/PA28 class
// from the SR22/DA62 class; business jets and airliners part company around 20 t.
constexpr float ULTRA_LIGHT_MAX_MASS_KG = 600.f;
constexpr float LIGHT_GA_MAX_MASS_KG    = 1500.f;
constexpr float VLJ_MAX_MASS_KG         = 20000.f;

// Last resort when even the catalog lacks the fallback profile.
constexpr DescentThresholds FIXED_WING_DEFAULT_THRESHOLDS = {-125, -250, -350, -600};
constexpr DescentThresholds ROTORCRAFT_DEFAULT_THRESHOLDS = {-75, -150, -300, -500};

} // namespace

std::string_view classify_fixed_wing_profile(const AircraftIdentity &aircraft)
{
    if (aircraft.max_takeoff_mass_kg <= 0.f)
    {
        return {};
    }
    switch (aircraft.engine_kind)
    {
    case EngineKind::Jet:
        return aircraft.max_takeoff_mass_kg <= VLJ_MAX_MASS_KG ? "vlj" : "heavy_jet";
    case EngineKind::Turboprop:
        return "turboprop";
    case EngineKind::Piston:
        break;
    }
    // A second piston engine makes it a twin whatever it weighs.
    if (aircraft.engine_count > 1)
    {
        return "medium_ga";
    }
    if (aircraft.max_takeoff_mass_kg <= ULTRA_LIGHT_MAX_MASS_KG)
    {
        return "ultra_light";
    }
    return aircraft.max_takeoff_mass_kg <= LIGHT_GA_MAX_MASS_KG ? "light_ga" : "medium_ga";
}

ProfileResolver::ProfileResolver(ProfileCatalog catalog) : catalog_(std::move(catalog)) {}

LandingProfile ProfileResolver::resolve(const AircraftIdentity &aircraft) const
{
    std::string name = listed_profile(aircraft.type_designator);
    if (name.empty() && !aircraft.is_rotorcraft)
    {
        name = std::string(classify_fixed_wing_profile(aircraft));
    }
    if (name.empty() || catalog_.profiles.count(name) == 0)
    {
        name = aircraft.is_rotorcraft ? ROTORCRAFT_FALLBACK_PROFILE : FIXED_WING_FALLBACK_PROFILE;
    }
    return named(name, aircraft.is_rotorcraft);
}

std::string ProfileResolver::listed_profile(const std::string &type_designator) const
{
    if (type_designator.empty())
    {
        return {};
    }
    for (const AircraftMatch &entry : catalog_.aircraft)
    {
        if (!entry.match.empty() && type_designator.find(entry.match) != std::string::npos)
        {
            return entry.profile;
        }
    }
    return {};
}

LandingProfile ProfileResolver::named(const std::string &name, bool is_rotorcraft) const
{
    const auto found = catalog_.profiles.find(name);
    if (found != catalog_.profiles.end())
    {
        return found->second;
    }
    return {.name          = name,
            .is_rotorcraft = is_rotorcraft,
            .thresholds    = is_rotorcraft ? ROTORCRAFT_DEFAULT_THRESHOLDS : FIXED_WING_DEFAULT_THRESHOLDS};
}

} // namespace fs24::domain
