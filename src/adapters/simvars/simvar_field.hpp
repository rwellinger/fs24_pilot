/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include <array>
#include <cstdint>

// SDK-free description of a SimConnect data definition, so the mapping can be unit-tested
// on the host. The session adapter registers the fields in exactly this order.
namespace fs24::adapters::simvars
{

enum class FieldType : std::uint8_t
{
    Number,    // SIMCONNECT_DATATYPE_FLOAT64
    String128, // SIMCONNECT_DATATYPE_STRING128
};

struct SimVarField
{
    const char *name;
    const char *unit; // empty for strings
    FieldType   type = FieldType::Number;
};

} // namespace fs24::adapters::simvars
