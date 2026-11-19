# M00 — Repo & build skeleton

GitHub: issue #1, milestone "M00 — Repo & build skeleton"

## Goal

Build pipeline end to end before any feature: an (almost) empty WASM standalone module is built on the GitHub Windows runner, packaged without `fspackagetool`, installed into `Community/` and loads in MSFS 2024.

## Scope

- Repo layout per `docs/architecture.md` (`src/domain`, `src/application`, `src/adapters`, `wasm/`, `panel/`, `tests/`).
- `scripts/fetch-sdk.sh` (from fs24_streamdeck, python instead of node), `wasm/build.sh` (flags from fs24_streamdeck), `scripts/build_package.py`.
- Host CMake build + Catch2, clang-format/clang-tidy, `make check`.
- `.github/workflows/build.yml`: host check (macOS) + WASM build (windows-2025, SDK in Actions cache), artifact `fs24-pilot`.
- `module_init` logs `[fs24-pilot] <version> ready`.

## Out of scope

Any landing logic, panel.

## Acceptance criteria

- [x] `make check` green on the Mac.
- [x] `make package` builds on the Mac (compile check, stock LLVM).
- [x] `make gh-build` green; artifact contains `manifest.json`, `layout.json`, `modules/fs24-pilot.wasm`. — run 44479783132 (SDK 1.8.2, Asobo LLVM).
- [x] Sim PC: package in Community → Dev Mode WASM Debug shows the module loaded, console shows `[fs24-pilot] 0.1.0 ready`. — verified 2026-11-19: WASM Debug status READY, log clean.
