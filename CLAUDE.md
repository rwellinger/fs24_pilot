# fs24_pilot — Landing Rating for MSFS 2024

## Goal

Port the landing rating of `~/Workspace/x-plane/xp_pilot` (X-Plane 12) to Microsoft Flight Simulator 2024 as a single Community package.

**Hard product rule: no external app, no daemon.** The user unzips one package into `Community/` and nothing else runs as its own process. The package contains:

1. **WASM standalone module** (C++, `wasm/` + `src/`) — samples sim variables every sim frame, detects touchdowns, rates them, persists the last N landings, publishes results.
2. **In-game toolbar panel** (HTML/JS, `panel/`) — displays the result. Receives data from the module via CommBus. Contains no rating logic. Its JS only runs while the panel is open, so the module owns all state and answers `panel_ready` (`docs/research/panel.md`).
   - `panel/package/` — laid out exactly as in the package (`html_ui/…`).
   - `panel/devmode/` — Dev Mode project that compiles the `.spb`; its output `Packages/fs24-pilot-panel-build/InGamePanels/fs24-pilot-panel.spb` is committed and copied into the package. Rebuild only when the panel XML changes (procedure in `docs/research/panel.md`).

Sibling projects:
- `~/Workspace/x-plane/xp_pilot` — source of the landing logic. Code is **copied once and cleaned up** (not forked, no shared library). Known bug there: `pitch_deg` is fed from `Q` (pitch *rate*), `flight_logger.cpp:653–654` — do not port it.
- `~/Workspace/msfs2024/fs24_streamdeck` — source of the WASM toolchain, SDK fetch, packaging and CI. Its `docs/research/wasm-bridge.md` holds the hard-won toolchain findings (only Asobo's LLVM produces loadable modules).

Feasibility and risks: `docs/research/feasibility.md`. Architecture: `docs/architecture.md`.

## Architecture — Clean Architecture

The dependency rule points inward. FS24/SDK updates must only touch adapters.

| Ring | Path | May include | Tested |
|---|---|---|---|
| domain | `src/domain/` | std only | Catch2, host |
| application | `src/application/` | domain | Catch2, host, fakes for ports |
| adapters | `src/adapters/<name>/` | application, domain, MSFS SDK | smoke test in the sim |
| composition root | `wasm/src/module.cpp` | everything | sim |
| panel | `panel/` | CommBus protocol only (`docs/protocol.md`) | sim |

- The host CMake build never sees the SDK → an SDK include in domain/application fails the build. Keep it that way.
- **SimVar names and units exist only in `src/adapters/simvars/`** (anti-corruption layer). Domain types use SI-ish units with explicit names (`fpm`, `kts`, `m`).
- Ports (pure virtual interfaces in `application`) only where a second implementation is realistic: test fake or sim API change. No abstraction on spec.
- Configuration (profiles, thresholds) is injected, no globals/singletons.
- WASM ↔ panel messages are versioned JSON (`protocol_version`), documented in `docs/protocol.md`.

## Tech Stack

- C++20, `-fno-exceptions` (WASM constraint; the host build uses the same flag for production code). No `std::thread`.
- Host build: CMake + Catch2 amalgamated (`vendor/catch2`, BSL-1.0).
- WASM build: `wasm/build.sh` — flags copied from fs24_streamdeck, compiles `wasm/src/module.cpp` plus every `src/**/*.cpp`.
- Packaging: `scripts/build_package.py` (stdlib only) writes `manifest.json` + `layout.json`, no `fspackagetool`.
- Panel: plain HTML/CSS/JS (Coherent GT), no framework, no bundler unless M04 proves it necessary.
- Python 3 is only a build tool (SDK fetch, packaging). No npm in this repo.

## Development Setup — the sim PC stays clean

| Task | Where |
|---|---|
| Domain/application code + tests | Mac: `make test` |
| WASM compile check | Mac: `make wasm` (stock LLVM, not loadable) |
| Loadable build | `make gh-build` → GitHub Actions Windows runner with the SDK's LLVM |
| Install | sim PC: unzip the artifact into the Community folder |
| Panel `.spb` (rare) | sim PC: Dev Mode Project Editor on `panel/devmode/` of a repo checkout — the only sanctioned exception, see `docs/research/panel.md` |
| Debug | MSFS Developer Mode: console (`[fs24-pilot]` lines), WASM Debug window |

No Node, Visual Studio, MSFS SDK or any dev tool on the sim PC. Community folder (MS Store): `%LOCALAPPDATA%\Packages\Microsoft.Limitless_8wekyb3d8bbwe\LocalCache\Packages\Community`.

`SDK_DIR=<path>` reuses an already extracted SDK (e.g. `../fs24_streamdeck/sdk/current`) instead of `make sdk`.

### MSFS SDK licensing

SDK headers/libs are **never** committed (`/sdk/` is gitignored). The repo is public. CI keeps the SDK in the Actions cache.

### Licensing of ported code

xp_pilot is GPL-3.0, this repo is MIT. Ported code is relicensed by its sole author (thWelly). Do not port third-party code from xp_pilot (vendor/, other contributors) without checking its licence.

## Working Style

- **One milestone per session.** Each `.claude/tasks/Mxx-*.md` is self-contained and mirrors its GitHub issue (milestone of the same name). Re-read it; don't rely on earlier session context.
- **Issue first:** commits/PRs reference the milestone issue (`Refs #N` / `Closes #N`).
- **English** in code, comments, docs and task files. Communication with the user is German.
- **Clean Code:** small functions, intention-revealing names, no dead code, no commented-out code. Default to no comments — only the non-obvious *why*.
- **Tests first for domain/application.** Port the matching xp_pilot Catch2 cases along with the code.
- **Pre-commit gate:** `make check` (clang-format, clang-tidy, tests) must be green.
- **Verification before claiming done.** Run each milestone's verification steps; sim-side checks are done by the user on the sim PC.

## MSFS notes

- SimConnect IDs are connection-scoped; the module opens its own SimConnect connection in `module_init`.
- Logging: `fprintf(stdout|stderr, "[fs24-pilot] …")` → Dev Mode console.
- Useful SimVars: `PLANE TOUCHDOWN NORMAL VELOCITY` / `PLANE TOUCHDOWN PITCH DEGREES` / `PLANE TOUCHDOWN BANK DEGREES` (sim's own touchdown capture, cross-check), `CONTACT POINT IS ON GROUND:n`, `SIM ON GROUND`, `ATC MODEL` (icao_type_designator), `ENGINE TYPE`, `MAX GROSS WEIGHT`.

## References

- MSFS 2024 SDK docs: <https://docs.flightsimulator.com/msfs2024/html/Introduction/Introduction.htm>
- CommBus: `MSFS/MSFS_CommBus.h` (SDK), JS side `RegisterCommBusListener`.
- SimConnect Facilities API: `SimConnect_AddToFacilityDefinition` / `RequestFacilityData`.
