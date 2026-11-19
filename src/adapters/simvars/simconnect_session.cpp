/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "adapters/simvars/simconnect_session.hpp"

#include <chrono>
#include <cstdio>
#include <utility>

namespace fs24::adapters::simvars
{

namespace
{

enum Definition : SIMCONNECT_DATA_DEFINITION_ID
{
    DEF_FRAME    = 1,
    DEF_AIRCRAFT = 2,
};

enum Request : SIMCONNECT_DATA_REQUEST_ID
{
    REQ_FRAME     = 1,
    REQ_AIRCRAFT  = 2,
    REQ_SIM_STATE = 3,
};

enum Event : SIMCONNECT_CLIENT_EVENT_ID
{
    EVT_SIM              = 1,
    EVT_PAUSE            = 2,
    EVT_AIRCRAFT_LOADED  = 3,
    EVT_FLIGHT_LOADED    = 4,
    EVT_POSITION_CHANGED = 5,
};

template <std::size_t N> void add_fields(HANDLE sim, Definition definition, const std::array<SimVarField, N> &fields)
{
    for (const SimVarField &field : fields)
    {
        const bool is_string = field.type == FieldType::String128;
        SimConnect_AddToDataDefinition(sim, definition, field.name, is_string ? nullptr : field.unit,
                                       is_string ? SIMCONNECT_DATATYPE_STRING128 : SIMCONNECT_DATATYPE_FLOAT64);
    }
}

double steady_seconds()
{
    using clock = std::chrono::steady_clock;
    return std::chrono::duration<double>(clock::now().time_since_epoch()).count();
}

} // namespace

SimConnectSession::SimConnectSession(SessionHandlers handlers) : handlers_(std::move(handlers)) {}

SimConnectSession::~SimConnectSession()
{
    if (sim_ != 0)
    {
        SimConnect_Close(sim_);
    }
}

bool SimConnectSession::open()
{
    if (FAILED(SimConnect_Open(&sim_, "fs24-pilot", nullptr, 0, 0, 0)))
    {
        std::fprintf(stderr, "[fs24-pilot] SimConnect_Open failed\n");
        sim_ = 0;
        return false;
    }
    sim_time_var_ = fsVarsGetEVarId("SIMULATION TIME");
    seconds_unit_ = fsVarsGetUnitId("seconds");
    if (sim_time_var_ == FS_VAR_INVALID_ID || seconds_unit_ == FS_INVALID_UNIT)
    {
        std::fprintf(stderr, "[fs24-pilot] E:SIMULATION TIME unavailable, falling back to the wall clock\n");
    }
    register_definitions();
    subscribe_events();
    SimConnect_RequestSystemState(sim_, REQ_SIM_STATE, "Sim");
    request_aircraft();
    SimConnect_CallDispatch(sim_, dispatch, this);
    return true;
}

void SimConnectSession::register_definitions()
{
    add_fields(sim_, DEF_FRAME, FRAME_FIELDS);
    add_fields(sim_, DEF_AIRCRAFT, AIRCRAFT_FIELDS);
    SimConnect_RequestDataOnSimObject(sim_, REQ_FRAME, DEF_FRAME, SIMCONNECT_OBJECT_ID_USER_AIRCRAFT,
                                      SIMCONNECT_PERIOD_SIM_FRAME);
}

void SimConnectSession::subscribe_events()
{
    SimConnect_SubscribeToSystemEvent(sim_, EVT_SIM, "Sim");
    SimConnect_SubscribeToSystemEvent(sim_, EVT_PAUSE, "Pause_EX1");
    SimConnect_SubscribeToSystemEvent(sim_, EVT_AIRCRAFT_LOADED, "AircraftLoaded");
    SimConnect_SubscribeToSystemEvent(sim_, EVT_FLIGHT_LOADED, "FlightLoaded");
    SimConnect_SubscribeToSystemEvent(sim_, EVT_POSITION_CHANGED, "PositionChanged");
}

void SimConnectSession::request_aircraft()
{
    SimConnect_RequestDataOnSimObject(sim_, REQ_AIRCRAFT, DEF_AIRCRAFT, SIMCONNECT_OBJECT_ID_USER_AIRCRAFT,
                                      SIMCONNECT_PERIOD_ONCE);
}

double SimConnectSession::sim_time_s() const
{
    double seconds = 0.0;
    if (sim_time_var_ != FS_VAR_INVALID_ID &&
        fsVarsEVarGet(sim_time_var_, seconds_unit_, &seconds) == FS_VAR_ERROR_NONE)
    {
        return seconds;
    }
    return steady_seconds();
}

void CALLBACK SimConnectSession::dispatch(SIMCONNECT_RECV *data, DWORD /*size*/, void *ctx)
{
    static_cast<SimConnectSession *>(ctx)->handle(data);
}

void SimConnectSession::handle(SIMCONNECT_RECV *data)
{
    switch (data->dwID)
    {
    case SIMCONNECT_RECV_ID_SIMOBJECT_DATA:
        handle_data(*static_cast<SIMCONNECT_RECV_SIMOBJECT_DATA *>(data));
        return;
    case SIMCONNECT_RECV_ID_EVENT:
    case SIMCONNECT_RECV_ID_EVENT_FILENAME:
        handle_event(*static_cast<SIMCONNECT_RECV_EVENT *>(data));
        return;
    case SIMCONNECT_RECV_ID_SYSTEM_STATE:
    {
        const auto &system_state = *static_cast<SIMCONNECT_RECV_SYSTEM_STATE *>(data);
        if (system_state.dwRequestID == REQ_SIM_STATE)
        {
            state_.running = system_state.dwInteger != 0;
        }
        return;
    }
    case SIMCONNECT_RECV_ID_EXCEPTION:
    {
        const auto &exception = *static_cast<SIMCONNECT_RECV_EXCEPTION *>(data);
        std::fprintf(stderr, "[fs24-pilot] SimConnect exception %lu (send id %lu, index %lu)\n",
                     static_cast<unsigned long>(exception.dwException), static_cast<unsigned long>(exception.dwSendID),
                     static_cast<unsigned long>(exception.dwIndex));
        return;
    }
    default:
        return;
    }
}

void SimConnectSession::handle_event(const SIMCONNECT_RECV_EVENT &event)
{
    switch (event.uEventID)
    {
    case EVT_SIM:
        state_.running = event.dwData != 0;
        return;
    case EVT_PAUSE:
        state_.paused = event.dwData != 0;
        return;
    case EVT_AIRCRAFT_LOADED:
    case EVT_FLIGHT_LOADED:
    case EVT_POSITION_CHANGED:
        request_aircraft();
        return;
    default:
        return;
    }
}

void SimConnectSession::handle_data(const SIMCONNECT_RECV_SIMOBJECT_DATA &data)
{
    if (data.dwRequestID == REQ_FRAME)
    {
        state_.sim_time_s = sim_time_s();
        handlers_.on_frame(*reinterpret_cast<const RawFrame *>(&data.dwData), state_);
    }
    else if (data.dwRequestID == REQ_AIRCRAFT)
    {
        handlers_.on_aircraft(*reinterpret_cast<const RawAircraft *>(&data.dwData));
    }
}

} // namespace fs24::adapters::simvars
