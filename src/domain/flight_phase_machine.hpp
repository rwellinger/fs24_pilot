/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#pragma once

#include <cstdint>
#include <optional>

#include "domain/frame.hpp"
#include "domain/landing.hpp"
#include "domain/landing_detector.hpp"

namespace fs24::domain
{

inline constexpr float GROUND_ROLL_KTS = 30.f;
inline constexpr float AGL_AIRBORNE_M  = 15.f; // ~50 ft, fixed-wing climb-out
// A freshly placed aircraft drops onto the ground; for a helicopter that drop would be a
// flight with a hard landing, so lift-off only counts after standing this long.
inline constexpr float ROTORCRAFT_GROUND_SETTLE_S   = 3.f;
inline constexpr float ROTORCRAFT_LIFTOFF_CONFIRM_S = 2.f;
// Longer gaps between active frames (a stutter, a reposition) are not counted as time.
inline constexpr float MAX_FRAME_GAP_S = 0.25f;

enum class FlightPhase : std::uint8_t
{
    Idle,
    Rolling,
    Airborne,
    Landed,
};

enum class PhaseTransition : std::uint8_t
{
    None,
    StartedRoll,
    TookOff,
    LiftedOffRotorcraft,
    Landed,
    TookOffAgain, // touch-and-go, or a new departure after a landing
};

struct PhaseStep
{
    PhaseTransition        transition = PhaseTransition::None;
    std::optional<Landing> landing; // set with PhaseTransition::Landed
};

const char *transition_label(PhaseTransition transition);

// When an aircraft rolls, takes off, lands and goes again. At most one transition per
// frame. Fed only with active frames.
class FlightPhaseMachine
{
  public:
    // A new aircraft or a reposition: whatever was in progress belongs to the old one.
    void reset();

    PhaseStep step(const FrameInput &frame, bool is_rotorcraft);

    FlightPhase            phase() const { return phase_; }
    const LandingDetector &detector() const { return detector_; }
    bool                   settled_since_placed() const { return settled_since_placed_; }

  private:
    void      track_ground_contact(const FrameInput &frame);
    bool      rotorcraft_lifted_off() const;
    PhaseStep step_idle(const FrameInput &frame, bool is_rotorcraft);
    PhaseStep step_rolling(const FrameInput &frame);
    PhaseStep step_airborne(const FrameInput &frame, bool is_rotorcraft);
    PhaseStep step_landed(const FrameInput &frame, bool is_rotorcraft);
    PhaseStep become_airborne(const FrameInput &frame, PhaseTransition transition);

    FlightPhase           phase_ = FlightPhase::Idle;
    LandingDetector       detector_;
    std::optional<double> previous_time_s_;
    float                 seconds_on_ground_    = 0;
    float                 seconds_off_ground_   = 0;
    bool                  settled_since_placed_ = false;
};

} // namespace fs24::domain
