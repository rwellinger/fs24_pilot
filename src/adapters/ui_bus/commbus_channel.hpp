/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include <functional>
#include <string>
#include <string_view>

namespace fs24::adapters::ui_bus
{

// Transport between module and panel over the MSFS CommBus. Knows event names
// and broadcast flags, nothing about message contents.
class CommBusChannel
{
  public:
    using MessageHandler = std::function<void(std::string_view message_json)>;

    explicit CommBusChannel(MessageHandler on_panel_message);
    ~CommBusChannel();

    CommBusChannel(const CommBusChannel &)            = delete;
    CommBusChannel &operator=(const CommBusChannel &) = delete;

    bool send_to_panel(const std::string &message_json) const;

  private:
    static void on_commbus(const char *buf, unsigned int size, void *ctx);

    MessageHandler on_panel_message_;
};

} // namespace fs24::adapters::ui_bus
