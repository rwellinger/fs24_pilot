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
#include <string_view>

// Module <-> panel message envelope, see docs/protocol.md.
namespace fs24::application::ui_protocol
{

inline constexpr int              PROTOCOL_VERSION = 1;
inline constexpr std::string_view TO_PANEL_EVENT   = "fs24.pilot.to_panel";
inline constexpr std::string_view TO_MODULE_EVENT  = "fs24.pilot.to_module";

std::string json_string(std::string_view text);

// payload_json must already be a valid JSON value.
std::string envelope(std::string_view type, std::string_view payload_json);

// Extracts "type" from a panel message; empty when absent or malformed.
std::string message_type(std::string_view message_json);

// Reads the first integer field `key` anywhere in `json`; empty when absent or not a number.
// Enough for the flat messages and files this module reads.
std::optional<int> int_field(std::string_view json, std::string_view key);

// Payload of the "hello" message: module version and the stored panel settings.
std::string hello_payload(std::string_view module_version, int ui_scale_percent);

} // namespace fs24::application::ui_protocol
