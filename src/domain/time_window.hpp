/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include <deque>

namespace fs24::domain
{

// Average over the samples of the last `span_s` seconds. Time-based rather than a fixed
// sample count, so the smoothing does not depend on the frame rate.
class TimeWindow
{
  public:
    explicit TimeWindow(double span_s) : span_s_(span_s) {}

    void push(float value, double time_s)
    {
        samples_.push_front({.value = value, .time_s = time_s});
        while (samples_.size() > 2 && samples_.front().time_s - samples_.back().time_s > span_s_)
        {
            samples_.pop_back();
        }
    }

    float average() const
    {
        if (samples_.empty())
        {
            return 0.f;
        }
        double sum = 0.0;
        for (const Sample &sample : samples_)
        {
            sum += sample.value;
        }
        return static_cast<float>(sum / static_cast<double>(samples_.size()));
    }

    double time_span_s() const { return samples_.size() < 2 ? 0.0 : samples_.front().time_s - samples_.back().time_s; }

    void clear() { samples_.clear(); }

  private:
    struct Sample
    {
        float  value  = 0;
        double time_s = 0;
    };

    double             span_s_;
    std::deque<Sample> samples_;
};

} // namespace fs24::domain
