# Feasibility — landing rating for MSFS 2024

Assessed 2026-11-18 against xp_pilot (main, 2026-11-12) and MSFS 2024 SDK 1.8.2.

## Verdict

Feasible. The rating logic can be taken over largely unchanged; the sim adapter, runway lookup and UI are new. The main risk is the toolbar panel's packaging → M01 is a spike with a go/no-go gate, done before any logic is ported.

| Area | Rating | Reason |
|---|---|---|
| Core logic | high, ~1:1 | `landing_detector.*`, `flight_logger_logic.hpp` (rating 197–312, profile matching), `flight_state_machine.*`, `runway_geometry.hpp` are plain C++17 without SDK and covered by Catch2 tests. `FlightStateMachine::Environment` (`flight_state_machine.hpp:76–97`) is the seam to the sim |
| WASM toolchain | proven | Same chain as fs24_streamdeck (Asobo LLVM on the Windows runner, `MSFS_WasmVersions.a`, script packaging) |
| Sim data | high | All inputs exist as SimVars; MSFS even captures touchdown values itself (`PLANE TOUCHDOWN NORMAL VELOCITY`, `…PITCH DEGREES`, `…BANK DEGREES`) |
| Runways | medium | No `apt.dat` / `XPLMFindNavAid`. Replacement: SimConnect Facilities API (airport + runways incl. displaced threshold), asynchronous |
| Persistence | high | File I/O in the package's `\work` folder from WASM |
| UI | **main risk** | see below |

## Risks

### R1 — toolbar panel packaging (M01)

In-game toolbar panels are declared in `InGamePanels/*.xml`, which Asobo's package tool compiles into an `.spb`. `fspackagetool` needs an installed sim → conflicts with "the sim PC stays clean" and with building on the Mac/CI.
Options to evaluate in M01, in order:
1. Find out whether MSFS 2024 still requires the `.spb` or accepts another declaration.
2. Generate the `.spb` with a script (format reverse-engineered by the community).
3. Produce it once with the package tool and commit the binary (our own artefact, no SDK content) — acceptable fallback if 1–2 fail.

Gate: if no option yields a working toolbar entry, re-decide (EFB app, or another display path) before M02.

### R2 — showing the result automatically

Unknown whether the module can open the panel on touchdown. Fallback: the panel stays docked/open and updates live via CommBus.

### R3 — frame-rate dependent smoothing

xp_pilot smooths over 30 frames. The port uses a time window (~0.5 s) so the result does not depend on FPS.

### R4 — no exceptions in WASM

Built with `-fno-exceptions`. JSON handling must not throw (nlohmann/json with `JSON_NOEXCEPTION`, or a small dedicated writer/reader for the ring buffer).

### R5 — aircraft identification

xp_pilot matches `acf_ICAO` substrings. MSFS: `ATC MODEL` usually holds the ICAO type designator, but add-ons vary → matching table plus airframe classification fallback (`ENGINE TYPE`, `MAX GROSS WEIGHT`).

## Scope taken over

Descent rate + G grading (worst-of), crosswind allowance, bounce/hop detection, float time, flare label (with real pitch), rotorcraft rating (bank, yaw rate, drift, run-on), runway placement, last-N history.

## Not taken over

Flight logging/track, HTML reports, logbook, summary, airspace/map, Auto QNH.
