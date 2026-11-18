# M03 — SimVar adapter and live detection

GitHub: issue #4, milestone "M03 — SimVar adapter and live detection"

## Goal

The module detects and rates real landings in MSFS 2024 and logs the result to the Dev Mode console.

## Scope

- `src/adapters/simvars/`: one SimConnect data definition, period `SIM_FRAME`; the only place with SimVar names/units. Map to `FrameInput`.
  - AGL (`PLANE ALT ABOVE GROUND MINUS CG` vs `PLANE ALT ABOVE GROUND` — decide, document), IAS, GS, VS, G (`G FORCE`), pitch, bank, pitch/yaw rate (body), heading true/mag, lat/lon, wind, flaps, gear, AP.
  - Gear edges: any/all gear on ground from `CONTACT POINT IS ON GROUND:n` (number of contact points from `CONTACT POINT IS ON GROUND` indices / aircraft type), skid handling for helicopters.
  - Pause / active pause / slew / replay gating (`SIM DISABLED`, `IS SLEW ACTIVE`, pause state via system events).
- `AircraftInfoProvider` adapter: `ATC MODEL`, `TITLE`, `ATC ID`, `ENGINE TYPE`, `NUMBER OF ENGINES`, `MAX GROSS WEIGHT`, category (helicopter) — refreshed on aircraft change.
- `Logger` adapter → stdout/stderr with `[fs24-pilot]` prefix; `Clock` → sim time.
- Composition root wires `LandingMonitor`.
- Cross-check: log `PLANE TOUCHDOWN NORMAL VELOCITY` next to our smoothed fpm.

## Out of scope

Panel display (M04), runway placement (M05).

## Acceptance criteria

- [ ] 5 landings with the C172 and 3 with an airliner: grade + fpm + G in the console; fpm within ±10 % of the sim's touchdown velocity (deviation documented).
- [ ] Bounce detected and counted on a provoked bounce.
- [ ] Touch-and-go yields two separate landings.
- [ ] H145 landing produces a rotorcraft rating.
- [ ] Pause during approach does not corrupt the result.
- [ ] No measurable FPS impact (WASM Debug callback times documented).
