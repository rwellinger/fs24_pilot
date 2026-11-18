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
   LandingMonitor   frame → state machine → rating → repository + publisher
   ports: RunwayProvider, LandingRepository, ResultPublisher, AircraftInfoProvider, Clock, Logger
        │
 src/domain/            (pure C++20, no I/O)
   LandingDetector, FlightPhaseMachine, RatingPolicy, ProfileResolver, RunwayGeometry,
   value types (Grade, Landing, Profile, FrameInput …)
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

## Mapping from xp_pilot

| xp_pilot | fs24_pilot |
|---|---|
| `landing_detector.*` | `domain/landing_detector.*` (time-based smoothing window instead of 30 frames) |
| `flight_logger_logic.hpp` rating + profile parts | `domain/rating_policy.*`, `domain/profile_resolver.*` (split by responsibility) |
| `flight_state_machine.*` | `domain/flight_phase_machine.*` (only the phases needed for landing/touch-and-go) |
| `runway_geometry.hpp` | `domain/runway_geometry.hpp` |
| `FlightStateMachine::Environment` | application ports |
| `flight_logger.cpp` `read_frame()` | `adapters/simvars` |
| `airport_lookup.cpp` + `apt.dat` | `adapters/facilities` |
| `flight_store.*` | `adapters/storage` (ring buffer only) |
| `ui_landing_popup.cpp` (ImGui) | `panel/` |
