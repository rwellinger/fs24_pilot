/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include "adapters/storage/panel_state_store.hpp"
#include "application/landing_message.hpp"
#include "application/ports.hpp"

namespace fs24::adapters::storage
{

// Remembers the published landing for the next sim start, then forwards it.
class LastLandingRecorder final : public application::ResultPublisher
{
  public:
    LastLandingRecorder(PanelStateStore &store, application::ResultPublisher &next) : store_(store), next_(next) {}

    void publish_landing(const domain::Landing &landing) override
    {
        store_.set_last_landing_payload(application::ui_protocol::landing_payload(landing));
        next_.publish_landing(landing);
    }

  private:
    PanelStateStore              &store_;
    application::ResultPublisher &next_;
};

} // namespace fs24::adapters::storage
