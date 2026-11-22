/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include <catch2/catch_amalgamated.hpp>

#include "application/ui_protocol.hpp"

namespace proto = fs24::application::ui_protocol;

TEST_CASE("envelope carries protocol version, type and payload")
{
    CHECK(proto::envelope("tick", "{\"count\":3}") == R"({"protocol_version":1,"type":"tick","payload":{"count":3}})");
}

TEST_CASE("json_string escapes quotes, backslashes and control characters")
{
    CHECK(proto::json_string("a\"b\\c\n") == R"("a\"b\\c\n")");
    CHECK(proto::json_string(std::string_view("\x01", 1)) == R"("\u0001")");
}

TEST_CASE("message_type reads the type of a panel message")
{
    CHECK(proto::message_type(R"({"protocol_version":1,"type":"panel_ready","payload":{}})") == "panel_ready");
    CHECK(proto::message_type(R"({ "type" : "x" })") == "x");
}

TEST_CASE("message_type is empty for malformed messages")
{
    CHECK(proto::message_type("").empty());
    CHECK(proto::message_type(R"({"payload":{}})").empty());
    CHECK(proto::message_type(R"({"type":)").empty());
}

TEST_CASE("int_field reads an integer field")
{
    CHECK(proto::int_field(R"({"type":"set_ui_scale","payload":{"percent":150}})", "percent") == 150);
    CHECK(proto::int_field(R"({ "percent" : -20 })", "percent") == -20);
}

TEST_CASE("int_field is empty for missing or non-integer fields")
{
    CHECK_FALSE(proto::int_field(R"({"payload":{}})", "percent"));
    CHECK_FALSE(proto::int_field(R"({"percent":"big"})", "percent"));
    CHECK_FALSE(proto::int_field(R"({"percent":1.5})", "percent"));
    CHECK_FALSE(proto::int_field(R"({"percent":)", "percent"));
}

TEST_CASE("hello payload carries version and panel scale")
{
    CHECK(proto::hello_payload("0.5.0", 150) == R"({"module_version":"0.5.0","ui_scale_percent":150})");
}
