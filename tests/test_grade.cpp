/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include <catch2/catch_amalgamated.hpp>

#include "domain/grade.hpp"

using fs24::domain::Grade;

TEST_CASE("grade labels follow the xp_pilot wording")
{
    CHECK(fs24::domain::label(Grade::Butter) == "BUTTER!");
    CHECK(fs24::domain::label(Grade::Wasted) == "WASTED!");
}

TEST_CASE("worse picks the harsher grade regardless of order")
{
    CHECK(fs24::domain::worse(Grade::Great, Grade::Hard) == Grade::Hard);
    CHECK(fs24::domain::worse(Grade::Hard, Grade::Great) == Grade::Hard);
    CHECK(fs24::domain::worse(Grade::Butter, Grade::Butter) == Grade::Butter);
}
