/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include <catch2/catch_amalgamated.hpp>

#include <string>

#include "application/landing_message.hpp"

using fs24::application::ui_protocol::landing_payload;

TEST_CASE("The landing payload carries the rating and metrics the panel shows", "[ui_protocol]")
{
    fs24::domain::Landing landing;
    landing.grade        = fs24::domain::Grade::Great;
    landing.descent_fpm  = -182.4f;
    landing.g_force      = 1.234f;
    landing.profile_name = "light_ga";
    landing.flare        = "Very good flare";

    const std::string json = landing_payload(landing);
    CHECK(json.front() == '{');
    CHECK(json.back() == '}');
    CHECK(json.find(R"("grade":"GREAT LANDING!")") != std::string::npos);
    CHECK(json.find(R"("grade_index":1)") != std::string::npos);
    CHECK(json.find(R"("descent_fpm":-182)") != std::string::npos);
    CHECK(json.find(R"("g_force":1.23)") != std::string::npos);
    CHECK(json.find(R"("runway":null)") != std::string::npos);
}

TEST_CASE("A placed landing carries its runway object", "[ui_protocol]")
{
    fs24::domain::Landing landing;
    landing.runway = fs24::domain::RunwayFix{
        .runway_ident = "14", .centerline_offset_m = -2.5f, .distance_from_thr_m = 410.f, .runway_length_m = 1527.f};
    const std::string json = landing_payload(landing);
    CHECK(json.find(R"("runway":{"ident":"14","distance_from_threshold_m":410,"centerline_offset_m":-2.5,)") !=
          std::string::npos);
}
