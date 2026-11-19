/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include <cstdio>
#include <string>
#include <string_view>

#include "application/ports.hpp"

namespace fs24::adapters::console
{

// Dev Mode console: everything on stdout carries the [fs24-pilot] prefix.
class ConsoleLogger final : public application::Logger
{
  public:
    void info(std::string_view message) override
    {
        std::fprintf(stdout, "[fs24-pilot] %.*s\n", static_cast<int>(message.size()), message.data());
    }
};

} // namespace fs24::adapters::console
