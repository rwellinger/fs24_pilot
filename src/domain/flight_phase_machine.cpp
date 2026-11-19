/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "domain/flight_phase_machine.hpp"

#include <algorithm>
#include <optional>
#include <utility>

namespace fs24::domain
{

const char *transition_label(PhaseTransition transition)
{
    switch (transition)
    {
    case PhaseTransition::StartedRoll:
        return "Idle -> Rolling";
    case PhaseTransition::TookOff:
        return "Rolling -> Airborne";
    case PhaseTransition::LiftedOffRotorcraft:
        return "Idle -> Airborne (rotorcraft)";
    case PhaseTransition::Landed:
        return "Airborne -> Landed";
    case PhaseTransition::TookOffAgain:
        return "Landed -> Airborne";
    case PhaseTransition::None:
        break;
    }
    return "";
}

void FlightPhaseMachine::reset() { *this = FlightPhaseMachine{}; }

PhaseStep FlightPhaseMachine::step(const FrameInput &frame, bool is_rotorcraft)
{
    track_ground_contact(frame);
    switch (phase_)
    {
    case FlightPhase::Idle:
        return step_idle(frame, is_rotorcraft);
    case FlightPhase::Rolling:
        return step_rolling(frame);
    case FlightPhase::Airborne:
        return step_airborne(frame, is_rotorcraft);
    case FlightPhase::Landed:
        return step_landed(frame, is_rotorcraft);
    }
    return {};
}

// Continuous ground and air time, which tells a hovering helicopter from one standing on
// an elevated pad — AGL alone cannot.
void FlightPhaseMachine::track_ground_contact(const FrameInput &frame)
{
    const float elapsed_s =
        previous_time_s_ ? std::clamp(static_cast<float>(frame.sim_time_s - *previous_time_s_), 0.f, MAX_FRAME_GAP_S)
                         : 0.f;
    previous_time_s_    = frame.sim_time_s;
    seconds_on_ground_  = frame.on_any_gear ? seconds_on_ground_ + elapsed_s : 0.f;
    seconds_off_ground_ = frame.on_any_gear ? 0.f : seconds_off_ground_ + elapsed_s;
    if (seconds_on_ground_ >= ROTORCRAFT_GROUND_SETTLE_S)
    {
        settled_since_placed_ = true;
    }
}

bool FlightPhaseMachine::rotorcraft_lifted_off() const { return seconds_off_ground_ >= ROTORCRAFT_LIFTOFF_CONFIRM_S; }

PhaseStep FlightPhaseMachine::step_idle(const FrameInput &frame, bool is_rotorcraft)
{
    if (is_rotorcraft)
    {
        // Helicopters skip the roll: the flight starts once the skids have left the ground.
        if (!settled_since_placed_ || !rotorcraft_lifted_off())
        {
            return {};
        }
        return become_airborne(frame, PhaseTransition::LiftedOffRotorcraft);
    }
    if (frame.ground_speed_kts <= GROUND_ROLL_KTS || !frame.on_any_gear)
    {
        return {};
    }
    phase_ = FlightPhase::Rolling;
    return {.transition = PhaseTransition::StartedRoll, .landing = std::nullopt};
}

PhaseStep FlightPhaseMachine::step_rolling(const FrameInput &frame)
{
    if (frame.agl_m <= AGL_AIRBORNE_M)
    {
        return {};
    }
    return become_airborne(frame, PhaseTransition::TookOff);
}

PhaseStep FlightPhaseMachine::step_airborne(const FrameInput &frame, bool is_rotorcraft)
{
    auto landing = is_rotorcraft ? detector_.step_rotorcraft(frame) : detector_.step_fixed_wing(frame);
    if (!landing)
    {
        return {};
    }
    phase_ = FlightPhase::Landed;
    return {.transition = PhaseTransition::Landed, .landing = std::move(landing)};
}

PhaseStep FlightPhaseMachine::step_landed(const FrameInput &frame, bool is_rotorcraft)
{
    const bool lifted_off = is_rotorcraft ? rotorcraft_lifted_off() : frame.agl_m > AGL_AIRBORNE_M;
    if (!lifted_off)
    {
        return {};
    }
    return become_airborne(frame, PhaseTransition::TookOffAgain);
}

PhaseStep FlightPhaseMachine::become_airborne(const FrameInput &frame, PhaseTransition transition)
{
    phase_ = FlightPhase::Airborne;
    detector_.arm(frame.on_any_gear, frame.on_all_gear);
    return {.transition = transition, .landing = std::nullopt};
}

} // namespace fs24::domain
