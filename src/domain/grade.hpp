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
#include <string_view>

namespace fs24::domain
{

// Ordered from best to worst; the rating is the worst grade of all criteria.
enum class Grade : std::uint8_t
{
    Butter,
    Great,
    Acceptable,
    Hard,
    Wasted,
};

inline constexpr std::size_t GRADE_COUNT = 5;

inline constexpr std::array<std::string_view, GRADE_COUNT> GRADE_LABELS = {
    "BUTTER!", "GREAT LANDING!", "ACCEPTABLE", "HARD LANDING!", "WASTED!",
};

constexpr std::string_view label(Grade grade) { return GRADE_LABELS[static_cast<std::size_t>(grade)]; }

constexpr Grade worse(Grade a, Grade b) { return static_cast<int>(a) >= static_cast<int>(b) ? a : b; }

} // namespace fs24::domain
