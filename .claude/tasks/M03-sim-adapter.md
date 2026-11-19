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

## Implementation notes (2026-11-19)

- Per-frame data: one SimConnect definition (`FRAME_FIELDS` in `adapters/simvars/frame_mapping.hpp`), period `SIM_FRAME`; sim time from `E:SIMULATION TIME` (wall-clock fallback logged).
- Active frame = `Sim` running ∧ no `Pause_EX1` flag ∧ not `IS SLEW ACTIVE`.
- Gear: any = `SIM ON GROUND` ∨ any `GEAR IS ON GROUND:0..2`; all = 0,1,2 (wheels) / both skids 10+11 or 1+2 (rotorcraft) — skid fallback: first ground contact when no gear flag is mapped.
- AGL: `PLANE ALT ABOVE GROUND MINUS CG` (meters).
- Aircraft: `ATC MODEL` (localisation key → designator), `CATEGORY`, `ENGINE TYPE`, `NUMBER OF ENGINES`, `MAX GROSS WEIGHT`; re-read on `AircraftLoaded` / `FlightLoaded` / `PositionChanged`.
- Profiles: xp_pilot profile classes, no match list yet (M06) → classification by airframe.
- **To verify in the sim:** pitch sign (MSFS nose-down positive → negated), body-X rate sign, bank sign, H145 skid flags. The console line `cross-check:` after each landing prints ours vs. the sim's own touchdown values and the raw gear flags.

## Sim results so far

- C172 #1 (0.2.0): ours −220 fpm, sim −242 fpm (9 %); pitch ours 9.3 vs sim raw −9.2 → `PITCH_SIGN = -1` confirmed; gear flags `111`; phases Idle → Rolling → Airborne → Landed; panel received the landing.
- G read 0.96 at touchdown (average over the flare) → switched to the peak after contact (0.3.0, see `docs/architecture.md`).
- `ATC MODEL` observed: `C172`, `DA42`, `C208`, `KODIAK`, `P46` (M500), `850` (TBM 850), `BE60` (Duke), `$$:PHENOM 300E` (FSReborn) → vendor free text, not ICAO. 0.3.1 logs `TITLE` with the aircraft line for M06 matching.
- 0.3.0 also drops the M01 tick and logs `aircraft … -> profile …` once per change.

## Sim test procedure

Free flight, Dev Mode console open (filter `fs24-pilot`), toolbar panel open.
1. On load: `aircraft <ICAO> -> profile <name>` — note the ICAO for C172, A320neo/737, H145, Kodiak.
2. Each landing logs `Airborne -> Landed`, `landing: <GRADE> <fpm> fpm <G> G …` and `cross-check: ours … sim …`. The panel shows the same in "last landing".
3. Send the console lines for: 5× C172, 3× airliner, 1 provoked bounce, 1 touch-and-go, 1× H145, 1 landing with a pause on short final.

## Acceptance criteria

- [ ] 5 landings with the C172 and 3 with an airliner: grade + fpm + G in the console; fpm within ±10 % of the sim's touchdown velocity (deviation documented).
- [ ] Bounce detected and counted on a provoked bounce.
- [ ] Touch-and-go yields two separate landings.
- [ ] H145 landing produces a rotorcraft rating.
- [ ] Pause during approach does not corrupt the result.
- [ ] No measurable FPS impact (WASM Debug callback times documented).
