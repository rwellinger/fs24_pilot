/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include <string>

#include "domain/landing.hpp"

namespace fs24::application::ui_protocol
{

// Payload of the "landing" message (docs/protocol.md): what the panel shows, already
// rated — the panel never computes anything.
std::string landing_payload(const domain::Landing &landing);

} // namespace fs24::application::ui_protocol
