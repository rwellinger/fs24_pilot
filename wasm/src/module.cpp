/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

// Composition root of the standalone module: the only place that knows every
// ring. It creates the adapters, wires them into the application use cases and
// owns their lifetime.
//
// M01 spike: a one-second tick and a hello reply prove the CommBus path to the
// panel in both directions. Replaced by LandingMonitor wiring in M03/M04.

#include <MSFS/MSFS.h>
#include <MSFS/MSFS_WindowsTypes.h>

// SimConnect.h relies on the Windows types above; keep it in its own include block.
#include <SimConnect.h>

#include <cstdio>
#include <memory>
#include <string>
#include <string_view>

#include "adapters/ui_bus/commbus_channel.hpp"
#include "application/ui_protocol.hpp"

#ifndef FS24_PILOT_VERSION
#error "FS24_PILOT_VERSION must be defined by the build"
#endif

namespace
{

namespace proto = fs24::application::ui_protocol;
using fs24::adapters::ui_bus::CommBusChannel;

enum SimEvent : SIMCONNECT_CLIENT_EVENT_ID
{
    EVENT_ONE_SECOND = 1,
};

HANDLE                          g_sim = 0;
std::unique_ptr<CommBusChannel> g_channel;
unsigned                        g_ticks = 0;

void send_hello()
{
    const std::string payload =
        "{\"module_version\":" + proto::json_string(FS24_PILOT_VERSION) + ",\"ticks\":" + std::to_string(g_ticks) + "}";
    g_channel->send_to_panel(proto::envelope("hello", payload));
}

void on_panel_message(std::string_view message)
{
    const std::string type = proto::message_type(message);
    std::fprintf(stdout, "[fs24-pilot] panel -> module: %s\n", type.c_str());
    if (type == "panel_ready")
    {
        send_hello();
    }
}

void on_one_second()
{
    ++g_ticks;
    g_channel->send_to_panel(proto::envelope("tick", "{\"count\":" + std::to_string(g_ticks) + "}"));
}

void CALLBACK dispatch(SIMCONNECT_RECV *data, DWORD /*size*/, void * /*ctx*/)
{
    if (data->dwID == SIMCONNECT_RECV_ID_EVENT &&
        static_cast<SIMCONNECT_RECV_EVENT *>(data)->uEventID == EVENT_ONE_SECOND)
    {
        on_one_second();
    }
}

} // namespace

extern "C" MSFS_CALLBACK void module_init(void)
{
    g_channel = std::make_unique<CommBusChannel>(on_panel_message);
    if (FAILED(SimConnect_Open(&g_sim, "fs24-pilot", nullptr, 0, 0, 0)))
    {
        std::fprintf(stderr, "[fs24-pilot] SimConnect_Open failed\n");
        return;
    }
    SimConnect_SubscribeToSystemEvent(g_sim, EVENT_ONE_SECOND, "1sec");
    SimConnect_CallDispatch(g_sim, dispatch, nullptr);
    std::fprintf(stdout, "[fs24-pilot] %s ready (protocol %d)\n", FS24_PILOT_VERSION, proto::PROTOCOL_VERSION);
}

extern "C" MSFS_CALLBACK void module_deinit(void)
{
    if (g_sim != 0)
    {
        SimConnect_Close(g_sim);
        g_sim = 0;
    }
    g_channel.reset();
    std::fprintf(stdout, "[fs24-pilot] stopped\n");
}
