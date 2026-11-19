/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "adapters/simvars/aircraft_mapping.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <string>
#include <string_view>

namespace fs24::adapters::simvars
{

namespace
{

constexpr float KG_PER_LB = 0.45359237f;

// ENGINE TYPE enum: 0 piston, 1 jet, 2 none, 3 helo (Bell) turbine, 4 unsupported, 5 turboprop.
constexpr int ENGINE_TYPE_JET          = 1;
constexpr int ENGINE_TYPE_HELO_TURBINE = 3;
constexpr int ENGINE_TYPE_TURBOPROP    = 5;

constexpr std::string_view MODEL_KEY = "AC_MODEL";

std::string_view bounded(const std::array<char, SIMVAR_STRING_SIZE> &text)
{
    return {text.data(), strnlen(text.data(), text.size())};
}

std::string trimmed_upper(std::string_view text)
{
    const auto first = text.find_first_not_of(" \t");
    if (first == std::string_view::npos)
    {
        return {};
    }
    text = text.substr(first, text.find_last_not_of(" \t") - first + 1);
    std::string out(text);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return out;
}

domain::EngineKind engine_kind(int engine_type)
{
    switch (engine_type)
    {
    case ENGINE_TYPE_JET:
        return domain::EngineKind::Jet;
    case ENGINE_TYPE_TURBOPROP:
    case ENGINE_TYPE_HELO_TURBINE:
        return domain::EngineKind::Turboprop;
    default:
        return domain::EngineKind::Piston;
    }
}

bool equals_ignoring_case(std::string_view a, std::string_view b)
{
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](unsigned char x, unsigned char y)
                                              { return std::tolower(x) == std::tolower(y); });
}

} // namespace

std::string type_designator_from_atc_model(std::string_view atc_model)
{
    const auto key = atc_model.find(MODEL_KEY);
    if (key == std::string_view::npos)
    {
        return trimmed_upper(atc_model.substr(0, atc_model.find('.')));
    }
    std::string_view rest = atc_model.substr(key + MODEL_KEY.size());
    if (!rest.empty() && (rest.front() == '_' || rest.front() == ' '))
    {
        rest.remove_prefix(1);
    }
    return trimmed_upper(rest.substr(0, rest.find('.')));
}

domain::AircraftIdentity to_identity(const RawAircraft &raw)
{
    const int engine_type = static_cast<int>(raw.engine_type);

    domain::AircraftIdentity identity;
    identity.type_designator = type_designator_from_atc_model(bounded(raw.atc_model));
    identity.is_rotorcraft =
        equals_ignoring_case(bounded(raw.category), "Helicopter") || engine_type == ENGINE_TYPE_HELO_TURBINE;
    identity.max_takeoff_mass_kg = static_cast<float>(raw.max_gross_weight_lbs) * KG_PER_LB;
    identity.engine_count        = static_cast<int>(raw.engine_count);
    identity.engine_kind         = engine_kind(engine_type);
    identity.title               = std::string(bounded(raw.title));
    return identity;
}

} // namespace fs24::adapters::simvars
