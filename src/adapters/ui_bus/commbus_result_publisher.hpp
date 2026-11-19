/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include "adapters/ui_bus/commbus_channel.hpp"
#include "application/ports.hpp"

namespace fs24::adapters::ui_bus
{

class CommBusResultPublisher final : public application::ResultPublisher
{
  public:
    explicit CommBusResultPublisher(const CommBusChannel &channel) : channel_(channel) {}

    void publish_landing(const domain::Landing &landing) override;

  private:
    const CommBusChannel &channel_;
};

} // namespace fs24::adapters::ui_bus
