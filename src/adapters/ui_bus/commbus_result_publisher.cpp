/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "adapters/ui_bus/commbus_result_publisher.hpp"

#include "application/landing_message.hpp"
#include "application/ui_protocol.hpp"

namespace fs24::adapters::ui_bus
{

void CommBusResultPublisher::publish_landing(const domain::Landing &landing)
{
    namespace proto = application::ui_protocol;
    channel_.send_to_panel(proto::envelope("landing", proto::landing_payload(landing)));
}

} // namespace fs24::adapters::ui_bus
