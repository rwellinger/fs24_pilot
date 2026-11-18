# M02 — Domain + application port with tests

GitHub: issue #3, milestone "M02 — Domain + application port with tests"

## Goal

The landing logic of xp_pilot runs as SDK-free domain/application code in this repo, covered by the ported Catch2 tests.

## Inputs

xp_pilot: `src/landing_detector.*`, `src/flight_logger_logic.hpp` (rating 197–312, profiles 58–195), `src/flight_state_machine.*`, `src/flight_session.hpp`, `src/runway_geometry.hpp`, tests `test_landing_detector.cpp`, `test_flight_logger_logic.cpp` (308–760), `test_flight_state_machine.cpp` (503–597), `test_runway.cpp` (geometry part).

## Scope

### Domain (`src/domain/`)
- `LandingDetector`: 50 ft gate (interpolated IAS/VS/flaps), float time, first/bounce/hop (bounce limit 5 ft), hardest bounce wins, completion on all-gear contact. **Time-based smoothing window (~0.5 s)** instead of 30 frames.
- `FlightPhaseMachine`: only the phases needed (ground → airborne → landed, touch-and-go, rotorcraft settle/lift-off confirmation). No shutdown/flight logging.
- `RatingPolicy`: fixed-wing (descent + G, crosswind allowance, climbing on contact = WASTED), rotorcraft (descent, bank, yaw rate, G, drift, run-on ≥ 15 kt).
- `ProfileResolver`: match table + airframe classification fallback; profiles are injected data.
- `RunwayGeometry`: copied from `runway_geometry.hpp`.
- Flare label uses **real pitch** and pitch rate (fixes the xp_pilot Q bug).
- Split `flight_logger_logic.hpp` by responsibility; drop track/VMC/map parts.

### Application (`src/application/`)
- `LandingMonitor` use case + ports `RunwayProvider`, `LandingRepository`, `ResultPublisher`, `AircraftInfoProvider`, `Clock`, `Logger`.
- Fakes for all ports in `tests/fakes/`.

## Out of scope

Any SDK code, JSON persistence format (M07), real profiles (M06 — use xp_pilot's bundled list as test data).

## Acceptance criteria

- [ ] All ported xp_pilot cases relevant to landing pass (list mapped cases in the issue).
- [ ] New tests: time-based smoothing independent of frame rate (20/60/120 fps give the same fpm ±2 %), pitch fix.
- [ ] `LandingMonitor` end-to-end test with synthetic frames and fakes.
- [ ] `make check` green; `make wasm` compiles the new sources for WASM (no exceptions, no threads).
