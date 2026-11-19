/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include "adapters/simvars/simvar_field.hpp"
#include "domain/profile.hpp"

// The aircraft SimVars, read once per loaded aircraft, and their translation into
// domain::AircraftIdentity.
namespace fs24::adapters::simvars
{

inline constexpr std::size_t SIMVAR_STRING_SIZE = 128;

// Numbers first, then strings, in AIRCRAFT_FIELDS order (SimConnect data block).
struct RawAircraft
{
    double                               engine_type;
    double                               engine_count;
    double                               max_gross_weight_lbs;
    std::array<char, SIMVAR_STRING_SIZE> atc_model;
    std::array<char, SIMVAR_STRING_SIZE> atc_type;
    std::array<char, SIMVAR_STRING_SIZE> title;
    std::array<char, SIMVAR_STRING_SIZE> category;
};

// NOLINTBEGIN(modernize-use-designated-initializers): a table reads better as {name, unit} rows.
inline constexpr std::array<SimVarField, 7> AIRCRAFT_FIELDS = {{
    {"ENGINE TYPE", "enum"},
    {"NUMBER OF ENGINES", "number"},
    {"MAX GROSS WEIGHT", "pounds"},
    {"ATC MODEL", "", FieldType::String128},
    {"ATC TYPE", "", FieldType::String128},
    {"TITLE", "", FieldType::String128},
    {"CATEGORY", "", FieldType::String128},
}};
// NOLINTEND(modernize-use-designated-initializers)

static_assert(sizeof(RawAircraft) == 3 * sizeof(double) + 4 * SIMVAR_STRING_SIZE,
              "RawAircraft must match AIRCRAFT_FIELDS");

// ATC MODEL often holds a localisation key ("TT:ATCCOM.AC_MODEL_C172.0.text") rather than
// the plain ICAO designator; this extracts the designator from either form.
std::string type_designator_from_atc_model(std::string_view atc_model);

domain::AircraftIdentity to_identity(const RawAircraft &raw);

} // namespace fs24::adapters::simvars
