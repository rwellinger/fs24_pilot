/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "application/landing_message.hpp"

#include <array>
#include <cstdio>
#include <optional>
#include <string>
#include <string_view>

#include "application/ui_protocol.hpp"
#include "domain/grade.hpp"

namespace fs24::application::ui_protocol
{

namespace
{

// Flat JSON object writer for the few value kinds the protocol uses.
class JsonObject
{
  public:
    JsonObject &add(std::string_view key, std::string_view text) { return raw(key, json_string(text)); }
    JsonObject &add(std::string_view key, bool flag) { return raw(key, flag ? "true" : "false"); }
    JsonObject &add(std::string_view key, int number) { return raw(key, std::to_string(number)); }
    JsonObject &add(std::string_view key, domain::Grade grade) { return add(key, static_cast<int>(grade)); }

    JsonObject &add(std::string_view key, std::optional<domain::Grade> grade)
    {
        return grade ? add(key, *grade) : raw(key, "null");
    }

    JsonObject &add(std::string_view key, double number, int decimals)
    {
        std::array<char, 32> buf{};
        std::snprintf(buf.data(), buf.size(), "%.*f", decimals, number);
        return raw(key, buf.data());
    }

    JsonObject &raw(std::string_view key, std::string_view json_value)
    {
        out_ += out_.size() == 1 ? "" : ",";
        out_ += json_string(key);
        out_ += ':';
        out_ += json_value;
        return *this;
    }

    std::string str() const { return out_ + '}'; }

  private:
    std::string out_ = "{";
};

std::string criteria_json(const domain::CriterionGrades &criteria)
{
    return JsonObject{}
        .add("descent", criteria.descent)
        .add("g_force", criteria.g_force)
        .add("drift", criteria.drift)
        .add("bank", criteria.bank)
        .add("yaw_rate", criteria.yaw_rate)
        .str();
}

std::string_view crosswind_side(float crosswind_kts) { return crosswind_kts < 0.f ? "left" : "right"; }

std::string runway_json(const domain::Landing &landing)
{
    if (!landing.runway)
    {
        return "null";
    }
    const domain::RunwayFix &fix = *landing.runway;
    return JsonObject{}
        .add("ident", fix.runway_ident)
        .add("distance_from_threshold_m", fix.distance_from_thr_m, 0)
        .add("centerline_offset_m", fix.centerline_offset_m, 1)
        .add("length_m", fix.runway_length_m, 0)
        .str();
}

} // namespace

std::string landing_payload(const domain::Landing &landing)
{
    return JsonObject{}
        .add("grade", domain::label(landing.grade))
        .add("grade_index", landing.grade)
        .raw("criteria", criteria_json(landing.criteria))
        .add("profile", landing.profile_name)
        .add("is_rotorcraft", landing.is_rotorcraft)
        .add("descent_fpm", landing.descent_fpm, 0)
        .add("g_force", landing.g_force, 2)
        .add("bounce_count", landing.bounce_count)
        .add("float_time_s", landing.float_time_s, 1)
        .add("ias_kts", landing.ias_kts, 0)
        .add("ground_speed_kts", landing.ground_speed_kts, 0)
        .add("pitch_deg", landing.pitch_deg, 1)
        .add("bank_deg", landing.bank_deg, 1)
        .add("yaw_rate_deg_s", landing.yaw_rate_deg_s, 1)
        .add("flare", landing.flare)
        .add("headwind_kts", landing.wind.headwind_kts, 0)
        .add("crosswind_kts", landing.wind.crosswind_kts, 0)
        .add("crosswind_side", crosswind_side(landing.wind.crosswind_kts))
        .add("wind_speed_kts", landing.wind_speed_kts, 0)
        .add("run_on", landing.is_rotorcraft && !landing.criteria.drift)
        .add("flap_ratio", landing.flap_ratio, 2)
        .add("gear_down", landing.gear_down)
        .add("autopilot_engaged", landing.autopilot_engaged)
        .add("airport", landing.airport_ident)
        .add("distance_to_surface_m", landing.distance_to_surface_m, 0)
        .add("off_field", landing.off_field)
        .raw("runway", runway_json(landing))
        .str();
}

} // namespace fs24::application::ui_protocol
