/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "adapters/facilities/simconnect_runway_provider.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <string>
#include <utility>

namespace fs24::adapters::facilities
{

namespace
{

constexpr SIMCONNECT_DATA_DEFINITION_ID DEF_AIRPORT = 1;
constexpr SIMCONNECT_DATA_REQUEST_ID    REQ_LIST    = 1;
// Every airport request gets its own id so data and end messages map back to it.
constexpr SIMCONNECT_DATA_REQUEST_ID REQ_FACILITY_FIRST = 100;

// The variable-length payload behind the fixed header of a facility data message.
struct Payload
{
    const std::byte *data = nullptr;
    std::size_t      size = 0;
};

Payload payload_of(const SIMCONNECT_RECV_FACILITY_DATA &message)
{
    const auto *start  = reinterpret_cast<const std::byte *>(&message);
    const auto *data   = reinterpret_cast<const std::byte *>(&message.Data);
    const auto  header = static_cast<std::size_t>(data - start);
    return {.data = data, .size = message.dwSize > header ? message.dwSize - header : 0};
}

} // namespace

SimConnectRunwayProvider::SimConnectRunwayProvider(application::Logger &logger, std::function<void()> on_lookup_done)
    : logger_(logger), on_lookup_done_(std::move(on_lookup_done)), next_request_id_(REQ_FACILITY_FIRST)
{
}

SimConnectRunwayProvider::~SimConnectRunwayProvider()
{
    if (sim_ != 0)
    {
        SimConnect_Close(sim_);
    }
}

bool SimConnectRunwayProvider::open()
{
    if (FAILED(SimConnect_Open(&sim_, "fs24-pilot-facilities", nullptr, 0, 0, 0)))
    {
        std::fprintf(stderr, "[fs24-pilot] SimConnect_Open for the runway lookup failed\n");
        sim_ = 0;
        return false;
    }
    for (const char *field : AIRPORT_DEFINITION)
    {
        SimConnect_AddToFacilityDefinition(sim_, DEF_AIRPORT, field);
    }
    SimConnect_CallDispatch(sim_, dispatch, this);
    return true;
}

void SimConnectRunwayProvider::prefetch_near(double latitude_deg, double longitude_deg)
{
    if (sim_ == 0)
    {
        return;
    }
    // The list is the sim's cache around the aircraft; the position only picks from it.
    lookup_latitude_deg_  = latitude_deg;
    lookup_longitude_deg_ = longitude_deg;
    if (list_pending_)
    {
        return;
    }
    listed_.clear();
    list_parts_   = 0;
    list_pending_ = true;
    SimConnect_RequestFacilitiesList_EX1(sim_, SIMCONNECT_FACILITY_LIST_TYPE_AIRPORT, REQ_LIST);
    list_send_id_ = last_send_id();
}

const domain::AirportSurfaces *SimConnectRunwayProvider::surfaces_near(double latitude_deg, double longitude_deg) const
{
    return cache_.nearest(latitude_deg, longitude_deg);
}

bool SimConnectRunwayProvider::lookup_pending() const { return list_pending_ || !fetching_.empty(); }

void CALLBACK SimConnectRunwayProvider::dispatch(SIMCONNECT_RECV *data, DWORD /*size*/, void *ctx)
{
    static_cast<SimConnectRunwayProvider *>(ctx)->handle(data);
}

void SimConnectRunwayProvider::handle(SIMCONNECT_RECV *data)
{
    switch (data->dwID)
    {
    case SIMCONNECT_RECV_ID_AIRPORT_LIST:
        on_airport_list(*static_cast<SIMCONNECT_RECV_AIRPORT_LIST *>(data));
        return;
    case SIMCONNECT_RECV_ID_FACILITY_DATA:
        on_facility_data(*static_cast<SIMCONNECT_RECV_FACILITY_DATA *>(data));
        return;
    case SIMCONNECT_RECV_ID_FACILITY_DATA_END:
        on_facility_end(static_cast<SIMCONNECT_RECV_FACILITY_DATA_END *>(data)->RequestId);
        return;
    case SIMCONNECT_RECV_ID_EXCEPTION:
        on_exception(*static_cast<SIMCONNECT_RECV_EXCEPTION *>(data));
        return;
    default:
        return;
    }
}

// Large lists arrive in several parts; the lookup goes on once all have been seen.
void SimConnectRunwayProvider::on_airport_list(const SIMCONNECT_RECV_AIRPORT_LIST &list)
{
    if (list.dwRequestID != REQ_LIST || !list_pending_)
    {
        return;
    }
    for (DWORD i = 0; i < list.dwArraySize; ++i)
    {
        const SIMCONNECT_DATA_FACILITY_AIRPORT &airport = list.rgData[i];
        listed_.push_back({.ident         = to_ident(airport.Ident, sizeof(airport.Ident)),
                           .latitude_deg  = airport.Latitude,
                           .longitude_deg = airport.Longitude});
    }
    if (++list_parts_ < std::max<DWORD>(list.dwOutOf, 1))
    {
        return;
    }
    list_pending_ = false;
    fetch_nearby();
    finish_lookup_if_done();
}

void SimConnectRunwayProvider::fetch_nearby()
{
    const std::vector<std::string> idents =
        nearby_idents(std::move(listed_), lookup_latitude_deg_, lookup_longitude_deg_);
    listed_.clear();
    for (const std::string &ident : idents)
    {
        if (cache_.contains(ident) || is_fetching(ident))
        {
            continue;
        }
        const DWORD request_id = next_request_id_++;
        SimConnect_RequestFacilityData(sim_, DEF_AIRPORT, request_id, ident.c_str(), "");
        fetching_.push_back({.ident = ident, .request_id = request_id, .send_id = last_send_id(), .assembler = {}});
    }
}

void SimConnectRunwayProvider::on_facility_data(const SIMCONNECT_RECV_FACILITY_DATA &message)
{
    Fetch *fetch = fetch_for(message.UserRequestId);
    if (fetch == nullptr)
    {
        return;
    }
    const Payload     payload   = payload_of(message);
    AirportAssembler &assembler = fetch->assembler;
    switch (message.Type)
    {
    case SIMCONNECT_FACILITY_DATA_AIRPORT:
        if (RawAirport raw{}; read_block(payload.data, payload.size, raw))
        {
            assembler.set_airport(raw);
        }
        return;
    case SIMCONNECT_FACILITY_DATA_RUNWAY:
        if (RawRunway raw{}; read_block(payload.data, payload.size, raw))
        {
            assembler.add_runway(message.UniqueRequestId, raw);
        }
        return;
    case SIMCONNECT_FACILITY_DATA_PAVEMENT:
        if (RawPavement raw{}; read_block(payload.data, payload.size, raw))
        {
            assembler.add_threshold(message.ParentUniqueRequestId, raw);
        }
        return;
    case SIMCONNECT_FACILITY_DATA_HELIPAD:
        if (RawHelipad raw{}; read_block(payload.data, payload.size, raw))
        {
            assembler.add_helipad(raw);
        }
        return;
    default:
        return;
    }
}

void SimConnectRunwayProvider::on_facility_end(DWORD request_id)
{
    Fetch *fetch = fetch_for(request_id);
    if (fetch == nullptr)
    {
        return;
    }
    logger_.info(fetch->assembler.describe());
    cache_.put(fetch->assembler.finish());
    fetching_.erase(fetching_.begin() + (fetch - fetching_.data()));
    finish_lookup_if_done();
}

// A refused request never sends its end message: drop it so the lookup can finish.
void SimConnectRunwayProvider::on_exception(const SIMCONNECT_RECV_EXCEPTION &exception)
{
    std::fprintf(stderr, "[fs24-pilot] runway lookup: SimConnect exception %lu (send id %lu)\n",
                 static_cast<unsigned long>(exception.dwException), static_cast<unsigned long>(exception.dwSendID));
    if (list_pending_ && exception.dwSendID == list_send_id_)
    {
        list_pending_ = false;
    }
    fetching_.erase(std::remove_if(fetching_.begin(), fetching_.end(),
                                   [&exception](const Fetch &fetch) { return fetch.send_id == exception.dwSendID; }),
                    fetching_.end());
    finish_lookup_if_done();
}

void SimConnectRunwayProvider::finish_lookup_if_done()
{
    if (!lookup_pending())
    {
        on_lookup_done_();
    }
}

bool SimConnectRunwayProvider::is_fetching(const std::string &ident) const
{
    return std::any_of(fetching_.begin(), fetching_.end(),
                       [&ident](const Fetch &fetch) { return fetch.ident == ident; });
}

SimConnectRunwayProvider::Fetch *SimConnectRunwayProvider::fetch_for(DWORD request_id)
{
    const auto it = std::find_if(fetching_.begin(), fetching_.end(),
                                 [request_id](const Fetch &fetch) { return fetch.request_id == request_id; });
    return it == fetching_.end() ? nullptr : &*it;
}

DWORD SimConnectRunwayProvider::last_send_id() const
{
    DWORD id = 0;
    SimConnect_GetLastSentPacketID(sim_, &id);
    return id;
}

} // namespace fs24::adapters::facilities
