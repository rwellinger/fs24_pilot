/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include <array>
#include <cstdio>
#include <functional>
#include <utility>

#include "adapters/simvars/frame_mapping.hpp"
#include "application/ports.hpp"

namespace fs24::adapters::simvars
{

// Logs the sim's own touchdown capture next to our rating, then forwards the landing.
// M03 verification aid: fpm deviation and the pitch sign convention.
class TouchdownCrossCheck final : public application::ResultPublisher
{
  public:
    TouchdownCrossCheck(std::function<const RawFrame &()> last_frame, application::ResultPublisher &next,
                        application::Logger &logger)
        : last_frame_(std::move(last_frame)), next_(next), logger_(logger)
    {
    }

    void publish_landing(const domain::Landing &landing) override
    {
        const RawFrame       &raw = last_frame_();
        std::array<char, 200> buf{};
        std::snprintf(buf.data(), buf.size(),
                      "cross-check: ours %.0f fpm, sim %.0f fpm; pitch ours %.1f, sim raw %.1f; gear c/l/r %d%d%d "
                      "skids %d%d",
                      static_cast<double>(landing.descent_fpm), -raw.touchdown_normal_velocity_fps * 60.0,
                      static_cast<double>(landing.pitch_deg), raw.touchdown_pitch_deg,
                      static_cast<int>(raw.gear_center_on_ground), static_cast<int>(raw.gear_left_on_ground),
                      static_cast<int>(raw.gear_right_on_ground), static_cast<int>(raw.skid_front_on_ground),
                      static_cast<int>(raw.skid_rear_on_ground));
        logger_.info(buf.data());
        next_.publish_landing(landing);
    }

  private:
    std::function<const RawFrame &()> last_frame_;
    application::ResultPublisher     &next_;
    application::Logger              &logger_;
};

} // namespace fs24::adapters::simvars
