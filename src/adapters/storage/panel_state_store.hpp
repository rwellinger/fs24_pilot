/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include <optional>
#include <string>

#include "application/ports.hpp"

namespace fs24::adapters::storage
{

inline constexpr int UI_SCALE_DEFAULT_PERCENT = 100;
inline constexpr int UI_SCALE_MIN_PERCENT     = 50;
inline constexpr int UI_SCALE_MAX_PERCENT     = 400;

// What the panel needs after a sim restart: its text size and the last landing (as the
// panel payload it was sent as). Two small files in `directory` (the package's \work folder
// in the sim), written whole on every change. Plain stdio: fopen is supported on \work.
class PanelStateStore
{
  public:
    PanelStateStore(std::string directory, application::Logger &logger);

    // Missing files are a first start; unreadable content falls back to the defaults.
    void load();

    int  ui_scale_percent() const { return ui_scale_percent_; }
    void set_ui_scale_percent(int percent);

    const std::optional<std::string> &last_landing_payload() const { return last_landing_payload_; }
    void                              set_last_landing_payload(std::string payload_json);

  private:
    std::string path(const char *file_name) const;
    void        write(const char *file_name, const std::string &content) const;

    std::string                directory_;
    application::Logger       &logger_;
    int                        ui_scale_percent_ = UI_SCALE_DEFAULT_PERCENT;
    std::optional<std::string> last_landing_payload_;
};

} // namespace fs24::adapters::storage
