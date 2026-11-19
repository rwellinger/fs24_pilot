/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include <string>

namespace fs24::domain
{

// Placement of a touchdown on the runway it was made on.
struct RunwayFix
{
    std::string runway_ident;
    float       centerline_offset_m = 0; // + = right of centerline
    float       distance_from_thr_m = 0; // measured from the displaced threshold
    float       runway_length_m     = 0; // displaced threshold to far end
};

} // namespace fs24::domain
