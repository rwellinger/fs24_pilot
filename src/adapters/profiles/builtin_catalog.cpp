/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "adapters/profiles/builtin_catalog.hpp"

namespace fs24::adapters::profiles
{

namespace
{

domain::LandingProfile fixed_wing(const char *name, domain::DescentThresholds thresholds)
{
    return {.name = name, .is_rotorcraft = false, .thresholds = thresholds};
}

domain::LandingProfile rotorcraft(const char *name, domain::DescentThresholds thresholds)
{
    return {.name = name, .is_rotorcraft = true, .thresholds = thresholds};
}

} // namespace

domain::ProfileCatalog builtin_catalog()
{
    domain::ProfileCatalog catalog;
    for (const domain::LandingProfile &profile : {
             fixed_wing("ultra_light", {-75, -150, -250, -400}),
             fixed_wing("light_ga", {-100, -200, -300, -500}),
             fixed_wing("medium_ga", {-125, -250, -350, -600}),
             fixed_wing("turboprop", {-150, -275, -400, -650}),
             fixed_wing("vlj", {-200, -350, -500, -750}),
             fixed_wing("heavy_jet", {-250, -400, -600, -850}),
             rotorcraft("light_helicopter", {-50, -120, -250, -400}),
             rotorcraft("turbine_helicopter", {-75, -150, -300, -500}),
         })
    {
        catalog.profiles.emplace(profile.name, profile);
    }
    return catalog;
}

} // namespace fs24::adapters::profiles
