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

// Flare quality from the pitch rate at main-gear contact: a steady attitude is a well
// timed flare; the nose still dropping means the flare came late, still rising early.
// Behaviour as in xp_pilot, which read the pitch rate (sim/flightmodel/position/Q).
std::string flare_label(float pitch_rate_deg_s);

} // namespace fs24::domain
