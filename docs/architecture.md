# Architecture

Clean Architecture with three rings plus a composition root and the panel. Goal: changes caused by FS24 updates (SimVar semantics, SDK API, panel API) stay inside adapters.

```
 panel/ (HTML/JS)            ◄── CommBus JSON (docs/protocol.md) ──┐
   Transport → ViewModel → View                                     │
                                                                    │
 wasm/src/module.cpp   composition root: creates + wires everything │
                                                                    │
 src/adapters/          (SDK-bound, replaceable)                    │
   simvars/     SimConnect data definitions → domain FrameInput     │
   facilities/  airport/runway lookup (Facilities API) → Runway     │
   storage/     landing ring buffer as JSON in the \work folder     │
   ui_bus/      ResultPublisher via fsCommBusCall ──────────────────┘
        │ implements ports
 src/application/       (use cases + ports)
   LandingMonitor   frame → phase machine → rating → placement → repository + publisher
   ports: AircraftInfoProvider, RunwayProvider, LandingRepository, ResultPublisher, Logger
        │
 src/domain/            (pure C++20, no I/O)
   LandingDetector, FlightPhaseMachine, rating_policy, ProfileResolver, runway_geometry,
   TimeWindow, flare, wind, value types (Grade, Landing, LandingProfile, FrameInput …)
```

## Rules

1. Dependencies point inward only. `domain` knows nothing else; `application` knows `domain`; adapters know both plus the SDK.
2. Enforced by the build: the host CMake target compiles domain/application without SDK include paths.
3. SimVar names, units and indices live only in `adapters/simvars`. Domain types use explicitly named units.
4. A port exists only where a second implementation is realistic (test fake, sim API change).
5. No globals. Profiles and thresholds are data (`data/profiles.json`, embedded at build time or read from the package) injected into the domain.
6. The panel is a dumb view: it renders what the protocol delivers and never re-computes a rating.

## Data flow per sim frame

1. `simvars` adapter receives `SIMCONNECT_RECV_SIMOBJECT_DATA` (period `SIM_FRAME`) and maps it to `FrameInput`.
2. `LandingMonitor::on_frame(FrameInput)` drives the phase machine and the detector.
3. On a completed landing: `RatingPolicy` grades it, `RunwayProvider` places it on the runway (async; result attached when available), `LandingRepository` stores it, `ResultPublisher` sends it to the panel.

## Decisions taken in M02

- **No Clock port:** `FrameInput::sim_time_s` carries the sim time; a port would have no second implementation.
- **Inactive frames** (pause, replay, slew) are dropped by `LandingMonitor` before they reach the domain — equivalent to xp_pilot freezing its counters.
- **Smoothing** over a 0.5 s time window (`TimeWindow`) instead of 30 frames; verified frame-rate independent (20/60/120 fps within 2 %).
- **Flare label** keeps xp_pilot's behaviour, which in fact rated the pitch *rate* (`Q`): |q| ≤ 1 °/s very good, nose dropping = late, rising = early. Fields are now named for what they hold; the real pitch attitude is recorded alongside. "Aggressive" is gone — it compared a rad/s value against 1 and never fired.
- **G force** (changed in M03 after the first sim landing): the peak of `G FORCE` within 0.3 s (`IMPACT_WINDOW_S`) after each main-gear / full-skid contact, not xp_pilot's 0.5 s average *before* contact — that average covers the flare and reads below 1 G (sim: 0.96 G at −220 fpm), so the G criterion never fired. The landing is emitted once the window has passed. Profile G thresholds still need checking against sim landings.
- **Bounce debounce** (M03): a lift of the mains counts only from 0.2 s airborne or 1 ft AGL. MSFS drops the contact flags for single frames on a paved runway; xp_pilot counted every lift.
- **Runway prefetch** also for rotorcraft (helipad distance); xp_pilot skipped it.
- **Not ported:** flight logging, shutdown detection, user profile overrides (possible later milestone), VMC/IMC.

## Mapping from xp_pilot

| xp_pilot | fs24_pilot |
|---|---|
| `landing_detector.*` | `domain/landing_detector.*` (time-based smoothing window instead of 30 frames) |
| `flight_logger_logic.hpp` rating + profile parts | `domain/rating_policy.*`, `domain/profile.*` (split by responsibility) |
| `flight_state_machine.*` | `domain/flight_phase_machine.*` (only the phases needed for landing/touch-and-go) |
| `runway_geometry.hpp` | `domain/runway_geometry.*` |
| `FlightStateMachine::Environment` | application ports |
| `flight_logger.cpp` `read_frame()` | `adapters/simvars` |
| `airport_lookup.cpp` + `apt.dat` | `adapters/facilities` |
| `flight_store.*` | `adapters/storage` (ring buffer only) |
| `ui_landing_popup.cpp` (ImGui) | `panel/` |
