/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "adapters/ui_bus/commbus_channel.hpp"

#include <MSFS/MSFS_CommBus.h>

#include <cstring>
#include <string>
#include <utility>

#include "application/ui_protocol.hpp"

namespace fs24::adapters::ui_bus
{

namespace proto = application::ui_protocol;

namespace
{

const std::string TO_PANEL_EVENT{proto::TO_PANEL_EVENT};
const std::string TO_MODULE_EVENT{proto::TO_MODULE_EVENT};

} // namespace

CommBusChannel::CommBusChannel(MessageHandler on_panel_message) : on_panel_message_(std::move(on_panel_message))
{
    fsCommBusRegister(TO_MODULE_EVENT.c_str(), &CommBusChannel::on_commbus, this);
}

CommBusChannel::~CommBusChannel()
{
    fsCommBusUnregisterOneEvent(TO_MODULE_EVENT.c_str(), &CommBusChannel::on_commbus, this);
}

bool CommBusChannel::send_to_panel(const std::string &message_json) const
{
    // The size includes the terminating NUL: the JS side receives a C string.
    return fsCommBusCall(TO_PANEL_EVENT.c_str(), message_json.c_str(),
                         static_cast<unsigned int>(message_json.size() + 1), FsCommBusBroadcast_JS);
}

void CommBusChannel::on_commbus(const char *buf, unsigned int size, void *ctx)
{
    auto *self = static_cast<CommBusChannel *>(ctx);
    if (self == nullptr || buf == nullptr)
    {
        return;
    }
    self->on_panel_message_(std::string_view(buf, strnlen(buf, size)));
}

} // namespace fs24::adapters::ui_bus
