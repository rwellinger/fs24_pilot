/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include <functional>

#include <MSFS/MSFS.h>
#include <MSFS/MSFS_Vars.h>
#include <MSFS/MSFS_WindowsTypes.h>

// SimConnect.h relies on the Windows types above; keep it in its own include block.
#include <SimConnect.h>

#include "adapters/simvars/aircraft_mapping.hpp"
#include "adapters/simvars/frame_mapping.hpp"

namespace fs24::adapters::simvars
{

struct SessionHandlers
{
    std::function<void(const RawFrame &, const SimState &)> on_frame;    // every sim frame
    std::function<void(const RawAircraft &)>                on_aircraft; // aircraft loaded / repositioned
    std::function<void()>                                   on_second;
};

// The module's own SimConnect connection: data definitions, system events and dispatch.
// Delivers raw SimVar blocks; translation into domain types happens in the *_mapping files.
class SimConnectSession
{
  public:
    explicit SimConnectSession(SessionHandlers handlers);
    ~SimConnectSession();

    SimConnectSession(const SimConnectSession &)            = delete;
    SimConnectSession &operator=(const SimConnectSession &) = delete;

    bool open();

  private:
    static void CALLBACK dispatch(SIMCONNECT_RECV *data, DWORD size, void *ctx);

    void   handle(SIMCONNECT_RECV *data);
    void   handle_event(const SIMCONNECT_RECV_EVENT &event);
    void   handle_data(const SIMCONNECT_RECV_SIMOBJECT_DATA &data);
    void   register_definitions();
    void   subscribe_events();
    void   request_aircraft();
    double sim_time_s() const;

    SessionHandlers handlers_;
    HANDLE          sim_ = 0;
    SimState        state_;
    FsEVarId        sim_time_var_ = FS_VAR_INVALID_ID;
    FsUnitId        seconds_unit_ = FS_INVALID_UNIT;
};

} // namespace fs24::adapters::simvars
