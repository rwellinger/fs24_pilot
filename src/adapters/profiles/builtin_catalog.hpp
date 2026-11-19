/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include "domain/profile.hpp"

namespace fs24::adapters::profiles
{

// The profile classes of xp_pilot without any aircraft match list: until the FS24 list
// lands (M06), every aircraft is classified by its airframe.
domain::ProfileCatalog builtin_catalog();

} // namespace fs24::adapters::profiles
