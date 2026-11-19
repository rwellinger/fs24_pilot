/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include <cstddef>
#include <deque>
#include <optional>

#include "application/ports.hpp"

namespace fs24::adapters::storage
{

// Keeps the most recent landings while the sim runs; the file-backed history follows in M07.
class MemoryLandingRepository final : public application::LandingRepository
{
  public:
    explicit MemoryLandingRepository(std::size_t capacity) : capacity_(capacity) {}

    void store(const domain::Landing &landing) override
    {
        landings_.push_front(landing);
        while (landings_.size() > capacity_)
        {
            landings_.pop_back();
        }
    }

    std::optional<domain::Landing> latest() const
    {
        return landings_.empty() ? std::nullopt : std::optional<domain::Landing>(landings_.front());
    }

  private:
    std::size_t                 capacity_;
    std::deque<domain::Landing> landings_;
};

} // namespace fs24::adapters::storage
