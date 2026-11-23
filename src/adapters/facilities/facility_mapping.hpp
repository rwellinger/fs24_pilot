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
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

#include "domain/runway_geometry.hpp"

// The Facilities API side of the runway lookup without the SDK: field names, the byte
// layout of the returned blocks and their translation into domain::AirportSurfaces.
namespace fs24::adapters::facilities
{

// One definition for an airport with its runways and helipads. The fields of each object
// arrive tightly packed in this order, which the Raw* blocks below mirror.
inline constexpr std::array<const char *, 29> AIRPORT_DEFINITION{
    "OPEN AIRPORT",
    "ICAO",
    "LATITUDE",
    "LONGITUDE",
    "MAGVAR",
    "OPEN RUNWAY",
    "LATITUDE",
    "LONGITUDE",
    "HEADING",
    "LENGTH",
    "WIDTH",
    "PRIMARY_NUMBER",
    "PRIMARY_DESIGNATOR",
    "SECONDARY_NUMBER",
    "SECONDARY_DESIGNATOR",
    "OPEN PRIMARY_THRESHOLD",
    "LENGTH",
    "ENABLE",
    "CLOSE PRIMARY_THRESHOLD",
    "OPEN SECONDARY_THRESHOLD",
    "LENGTH",
    "ENABLE",
    "CLOSE SECONDARY_THRESHOLD",
    "CLOSE RUNWAY",
    "OPEN HELIPAD",
    "LATITUDE",
    "LONGITUDE",
    "CLOSE HELIPAD",
    "CLOSE AIRPORT",
};

#pragma pack(push, 1)
struct RawAirport
{
    std::array<char, 8> icao;
    double              latitude_deg;
    double              longitude_deg;
    float               magvar_deg;
};

struct RawRunway
{
    double       latitude_deg; // centre of the runway
    double       longitude_deg;
    float        heading_deg; // of the primary end
    float        length_m;    // includes displaced thresholds, not blast pads or overruns
    float        width_m;
    std::int32_t primary_number; // 1-36, 37-44 = N, NE … NW
    std::int32_t primary_designator;
    std::int32_t secondary_number;
    std::int32_t secondary_designator;
};

struct RawPavement
{
    float        length_m;
    std::int32_t enabled;
};

struct RawHelipad
{
    double latitude_deg;
    double longitude_deg;
};
#pragma pack(pop)

// Copies a block out of a message buffer; false when the message is too short for it.
template <typename Raw> bool read_block(const void *data, std::size_t size, Raw &out)
{
    if (size < sizeof(Raw))
    {
        return false;
    }
    std::memcpy(&out, data, sizeof(Raw));
    return true;
}

// Fixed-size SimConnect strings are not terminated when they fill the field.
std::string to_ident(const char *chars, std::size_t capacity);

// "09", "27L", "N", "36W" from the Facilities API number and designator codes.
std::string runway_designator(int number, int designator);

domain::Runway to_runway(const RawRunway &raw, float primary_threshold_m, float secondary_threshold_m);

struct ListedAirport
{
    std::string ident;
    double      latitude_deg  = 0;
    double      longitude_deg = 0;
};

// Airports worth fetching for a landing near the given point: within reach of a final
// approach from 2000 ft, nearest first, capped so a dense area stays cheap.
inline constexpr float       NEARBY_AIRPORT_RADIUS_M = 10000.f;
inline constexpr std::size_t NEARBY_AIRPORTS_MAX     = 6;

std::vector<std::string> nearby_idents(std::vector<ListedAirport> airports, double latitude_deg, double longitude_deg);

// Collects the facility data messages of one airport. Children name their parent by its
// unique request id; a runway's two thresholds arrive in definition order.
class AirportAssembler
{
  public:
    void set_airport(const RawAirport &raw);
    void add_runway(std::uint32_t unique_id, const RawRunway &raw);
    void add_threshold(std::uint32_t parent_id, const RawPavement &raw);
    void add_helipad(const RawHelipad &raw);

    domain::AirportSurfaces finish() const;
    std::string             describe() const; // console line for the sim checks

  private:
    struct PendingRunway
    {
        std::uint32_t      unique_id = 0;
        RawRunway          raw{};
        std::vector<float> thresholds_m;
    };

    std::optional<RawAirport>  airport_;
    std::vector<PendingRunway> runways_;
    std::vector<RawHelipad>    helipads_;
};

} // namespace fs24::adapters::facilities
