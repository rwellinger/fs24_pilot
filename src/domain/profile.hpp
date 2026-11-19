/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "domain/rating_policy.hpp"

namespace fs24::domain
{

struct LandingProfile
{
    std::string       name;
    bool              is_rotorcraft = false;
    DescentThresholds thresholds{};
};

// Only the distinctions that change the landing profile; the rest is decided by mass.
enum class EngineKind : std::uint8_t
{
    Piston,
    Turboprop,
    Jet,
};

struct AircraftIdentity
{
    std::string type_designator; // ICAO type, e.g. "C172"
    bool        is_rotorcraft       = false;
    float       max_takeoff_mass_kg = 0; // <= 0 = unknown
    int         engine_count        = 0;
    EngineKind  engine_kind         = EngineKind::Piston;
};

struct AircraftMatch
{
    std::string match;   // substring of the type designator; first hit wins
    std::string profile; // name in ProfileCatalog::profiles
};

// The bundled profile data (data/profiles.json, M06).
struct ProfileCatalog
{
    std::map<std::string, LandingProfile> profiles;
    std::vector<AircraftMatch>            aircraft;
};

inline constexpr const char *FIXED_WING_FALLBACK_PROFILE = "medium_ga";
inline constexpr const char *ROTORCRAFT_FALLBACK_PROFILE = "turbine_helicopter";

// Profile class for an unlisted fixed-wing aircraft; empty when the mass is unknown.
std::string_view classify_fixed_wing_profile(const AircraftIdentity &aircraft);

class ProfileResolver
{
  public:
    explicit ProfileResolver(ProfileCatalog catalog);

    // Match list first, then airframe classification, then the category's fallback.
    LandingProfile resolve(const AircraftIdentity &aircraft) const;

  private:
    std::string    listed_profile(const std::string &type_designator) const;
    LandingProfile named(const std::string &name, bool is_rotorcraft) const;

    ProfileCatalog catalog_;
};

} // namespace fs24::domain
