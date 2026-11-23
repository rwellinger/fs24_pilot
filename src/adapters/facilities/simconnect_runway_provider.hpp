/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <MSFS/MSFS.h>
#include <MSFS/MSFS_WindowsTypes.h>

// SimConnect.h relies on the Windows types above; keep it in its own include block.
#include <SimConnect.h>

#include "adapters/facilities/facility_mapping.hpp"
#include "adapters/facilities/surface_cache.hpp"
#include "application/ports.hpp"

namespace fs24::adapters::facilities
{

// Runway lookup over its own SimConnect connection: the airports in the sim's facility
// cache around the aircraft, then runways and helipads of the nearest ones. Everything is
// asynchronous; answers arrive through the dispatch callback between frames.
class SimConnectRunwayProvider final : public application::RunwayProvider
{
  public:
    SimConnectRunwayProvider(application::Logger &logger, std::function<void()> on_lookup_done);
    ~SimConnectRunwayProvider() override;

    SimConnectRunwayProvider(const SimConnectRunwayProvider &)            = delete;
    SimConnectRunwayProvider &operator=(const SimConnectRunwayProvider &) = delete;

    bool open();

    void                           prefetch_near(double latitude_deg, double longitude_deg) override;
    const domain::AirportSurfaces *surfaces_near(double latitude_deg, double longitude_deg) const override;
    bool                           lookup_pending() const override;

  private:
    struct Fetch
    {
        std::string      ident;
        DWORD            request_id = 0;
        DWORD            send_id    = 0;
        AirportAssembler assembler;
    };

    static void CALLBACK dispatch(SIMCONNECT_RECV *data, DWORD size, void *ctx);

    void   handle(SIMCONNECT_RECV *data);
    void   on_airport_list(const SIMCONNECT_RECV_AIRPORT_LIST &list);
    void   on_facility_data(const SIMCONNECT_RECV_FACILITY_DATA &message);
    void   on_facility_end(DWORD request_id);
    void   on_exception(const SIMCONNECT_RECV_EXCEPTION &exception);
    void   fetch_nearby();
    void   finish_lookup_if_done();
    bool   is_fetching(const std::string &ident) const;
    Fetch *fetch_for(DWORD request_id);
    DWORD  last_send_id() const;

    static constexpr std::size_t AIRPORTS_CACHED = 32;

    application::Logger       &logger_;
    std::function<void()>      on_lookup_done_;
    HANDLE                     sim_ = 0;
    SurfaceCache               cache_{AIRPORTS_CACHED};
    bool                       list_pending_         = false;
    DWORD                      list_send_id_         = 0;
    std::uint32_t              list_parts_           = 0;
    double                     lookup_latitude_deg_  = 0;
    double                     lookup_longitude_deg_ = 0;
    std::vector<ListedAirport> listed_;
    std::vector<Fetch>         fetching_;
    DWORD                      next_request_id_;
};

} // namespace fs24::adapters::facilities
