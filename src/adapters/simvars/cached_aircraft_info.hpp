/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include <utility>

#include "adapters/simvars/aircraft_mapping.hpp"
#include "application/ports.hpp"

namespace fs24::adapters::simvars
{

// The identity of the loaded aircraft as last read from the sim.
class CachedAircraftInfo final : public application::AircraftInfoProvider
{
  public:
    void update(const RawAircraft &raw) { identity_ = to_identity(raw); }

    domain::AircraftIdentity current_aircraft() const override { return identity_; }

  private:
    domain::AircraftIdentity identity_;
};

} // namespace fs24::adapters::simvars
