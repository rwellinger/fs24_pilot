/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "application/ui_protocol.hpp"

#include <array>
#include <cstdio>
#include <string>
#include <string_view>

namespace fs24::application::ui_protocol
{

namespace
{

void append_escaped(std::string &out, char c)
{
    switch (c)
    {
    case '"':
        out += "\\\"";
        return;
    case '\\':
        out += "\\\\";
        return;
    case '\n':
        out += "\\n";
        return;
    case '\r':
        out += "\\r";
        return;
    case '\t':
        out += "\\t";
        return;
    default:
        break;
    }
    if (static_cast<unsigned char>(c) < 0x20)
    {
        std::array<char, 8> buf{};
        std::snprintf(buf.data(), buf.size(), "\\u%04x", static_cast<unsigned>(c));
        out += buf.data();
        return;
    }
    out += c;
}

} // namespace

std::string json_string(std::string_view text)
{
    std::string out;
    out.reserve(text.size() + 2);
    out += '"';
    for (const char c : text)
    {
        append_escaped(out, c);
    }
    out += '"';
    return out;
}

std::string envelope(std::string_view type, std::string_view payload_json)
{
    std::string out = "{\"protocol_version\":";
    out += std::to_string(PROTOCOL_VERSION);
    out += ",\"type\":";
    out += json_string(type);
    out += ",\"payload\":";
    out += payload_json;
    out += '}';
    return out;
}

std::string message_type(std::string_view message_json)
{
    constexpr std::string_view KEY     = "\"type\"";
    const auto                 key_pos = message_json.find(KEY);
    if (key_pos == std::string_view::npos)
    {
        return {};
    }
    const auto open = message_json.find('"', message_json.find(':', key_pos + KEY.size()));
    if (open == std::string_view::npos)
    {
        return {};
    }
    const auto close = message_json.find('"', open + 1);
    if (close == std::string_view::npos)
    {
        return {};
    }
    return std::string(message_json.substr(open + 1, close - open - 1));
}

} // namespace fs24::application::ui_protocol
