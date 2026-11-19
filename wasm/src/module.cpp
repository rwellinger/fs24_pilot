/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

// Composition root of the standalone module: the only place that knows every ring. It
// creates the adapters, wires them into the LandingMonitor use case and owns their
// lifetime. No logic lives here.

#include <MSFS/MSFS.h>

#include <cstdio>
#include <memory>
#include <string>
#include <string_view>

#include "adapters/console/console_logger.hpp"
#include "adapters/facilities/no_runway_data.hpp"
#include "adapters/profiles/builtin_catalog.hpp"
#include "adapters/simvars/cached_aircraft_info.hpp"
#include "adapters/simvars/frame_mapping.hpp"
#include "adapters/simvars/simconnect_session.hpp"
#include "adapters/simvars/touchdown_cross_check.hpp"
#include "adapters/storage/memory_landing_repository.hpp"
#include "adapters/ui_bus/commbus_channel.hpp"
#include "adapters/ui_bus/commbus_result_publisher.hpp"
#include "application/landing_message.hpp"
#include "application/landing_monitor.hpp"
#include "application/ui_protocol.hpp"

#ifndef FS24_PILOT_VERSION
#error "FS24_PILOT_VERSION must be defined by the build"
#endif

namespace
{

namespace proto = fs24::application::ui_protocol;
using namespace fs24::adapters;

constexpr std::size_t LANDINGS_KEPT = 20;

class Module
{
  public:
    Module()
        : channel_([this](std::string_view message) { on_panel_message(message); }), publisher_(channel_),
          repository_(LANDINGS_KEPT),
          cross_check_([this]() -> const simvars::RawFrame & { return last_frame_; }, publisher_, logger_),
          monitor_(fs24::domain::ProfileResolver(profiles::builtin_catalog()),
                   {aircraft_, runways_, repository_, cross_check_, logger_}),
          session_({.on_frame = [this](const simvars::RawFrame &raw, const simvars::SimState &state)
                    { on_frame(raw, state); },
                    .on_aircraft = [this](const simvars::RawAircraft &raw) { on_aircraft(raw); }})
    {
    }

    bool start() { return session_.open(); }

  private:
    void on_frame(const simvars::RawFrame &raw, const simvars::SimState &state)
    {
        last_frame_ = raw;
        monitor_.on_frame(simvars::to_frame(raw, state, aircraft_.current_aircraft().is_rotorcraft));
    }

    void on_aircraft(const simvars::RawAircraft &raw)
    {
        aircraft_.update(raw);
        monitor_.on_aircraft_loaded();
    }

    void on_panel_message(std::string_view message)
    {
        if (proto::message_type(message) != "panel_ready")
        {
            return;
        }
        channel_.send_to_panel(
            proto::envelope("hello", "{\"module_version\":" + proto::json_string(FS24_PILOT_VERSION) + "}"));
        if (const auto latest = repository_.latest())
        {
            channel_.send_to_panel(proto::envelope("landing", proto::landing_payload(*latest)));
        }
    }

    console::ConsoleLogger            logger_;
    ui_bus::CommBusChannel            channel_;
    ui_bus::CommBusResultPublisher    publisher_;
    storage::MemoryLandingRepository  repository_;
    facilities::NoRunwayData          runways_;
    simvars::CachedAircraftInfo       aircraft_;
    simvars::RawFrame                 last_frame_{};
    simvars::TouchdownCrossCheck      cross_check_;
    fs24::application::LandingMonitor monitor_;
    simvars::SimConnectSession        session_;
};

std::unique_ptr<Module> g_module;

} // namespace

extern "C" MSFS_CALLBACK void module_init(void)
{
    g_module = std::make_unique<Module>();
    if (!g_module->start())
    {
        g_module.reset();
        return;
    }
    std::fprintf(stdout, "[fs24-pilot] %s ready (protocol %d)\n", FS24_PILOT_VERSION, proto::PROTOCOL_VERSION);
}

extern "C" MSFS_CALLBACK void module_deinit(void)
{
    g_module.reset();
    std::fprintf(stdout, "[fs24-pilot] stopped\n");
}
