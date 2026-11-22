/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "adapters/storage/panel_state_store.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <optional>
#include <string>
#include <utility>

#include "application/ui_protocol.hpp"

namespace fs24::adapters::storage
{

namespace
{

constexpr const char *SETTINGS_FILE     = "settings.json";
constexpr const char *LAST_LANDING_FILE = "last_landing.json";
constexpr int         SETTINGS_VERSION  = 1;

std::optional<std::string> read_file(const std::string &path)
{
    std::FILE *file = std::fopen(path.c_str(), "rb");
    if (file == nullptr)
    {
        return std::nullopt;
    }
    std::string           content;
    std::array<char, 512> buf{};
    std::size_t           read = 0;
    while ((read = std::fread(buf.data(), 1, buf.size(), file)) > 0)
    {
        content.append(buf.data(), read);
    }
    std::fclose(file);
    return content;
}

bool looks_like_json_object(const std::string &text)
{
    const auto first = text.find_first_not_of(" \t\r\n");
    const auto last  = text.find_last_not_of(" \t\r\n");
    return first != std::string::npos && text[first] == '{' && text[last] == '}';
}

} // namespace

PanelStateStore::PanelStateStore(std::string directory, application::Logger &logger)
    : directory_(std::move(directory)), logger_(logger)
{
}

void PanelStateStore::load()
{
    namespace proto = application::ui_protocol;
    if (const auto settings = read_file(path(SETTINGS_FILE)))
    {
        const auto percent = proto::int_field(*settings, "ui_scale_percent");
        if (percent)
        {
            ui_scale_percent_ = std::clamp(*percent, UI_SCALE_MIN_PERCENT, UI_SCALE_MAX_PERCENT);
        }
        else
        {
            logger_.info("settings unreadable, using defaults");
        }
    }
    if (auto landing = read_file(path(LAST_LANDING_FILE)))
    {
        if (looks_like_json_object(*landing))
        {
            last_landing_payload_ = std::move(*landing);
        }
        else
        {
            logger_.info("last landing unreadable, ignored");
        }
    }
}

void PanelStateStore::set_ui_scale_percent(int percent)
{
    ui_scale_percent_ = std::clamp(percent, UI_SCALE_MIN_PERCENT, UI_SCALE_MAX_PERCENT);
    write(SETTINGS_FILE, "{\"version\":" + std::to_string(SETTINGS_VERSION) +
                             ",\"ui_scale_percent\":" + std::to_string(ui_scale_percent_) + "}\n");
}

void PanelStateStore::set_last_landing_payload(std::string payload_json)
{
    write(LAST_LANDING_FILE, payload_json);
    last_landing_payload_ = std::move(payload_json);
}

std::string PanelStateStore::path(const char *file_name) const { return directory_ + file_name; }

void PanelStateStore::write(const char *file_name, const std::string &content) const
{
    std::FILE *file    = std::fopen(path(file_name).c_str(), "wb");
    const bool written = file != nullptr && std::fwrite(content.data(), 1, content.size(), file) == content.size();
    const bool closed  = file != nullptr && std::fclose(file) == 0;
    if (!written || !closed)
    {
        logger_.info(std::string("could not write ") + path(file_name));
    }
}

} // namespace fs24::adapters::storage
