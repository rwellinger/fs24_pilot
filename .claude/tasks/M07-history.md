# M07 — Landing history (last N)

GitHub: issue #8, milestone "M07 — Landing history (last N)"

## Goal

The last 20 landings survive a sim restart and are listed in the panel.

## Scope

- `src/adapters/storage/`: `LandingRepository` as a JSON ring buffer in the package `\work` folder; schema with `version`; atomic write (temp + rename, if supported in WASM FS); no exceptions.
- Panel: list (time, airport/runway, aircraft, grade, fpm), select an entry to show its detail view; "request history" via CommBus panel → module (per M01 findings).
- Last landing is shown again after restart.

## Out of scope

Export, statistics, HTML reports.

## Acceptance criteria

- [ ] 21 landings → 20 stored, oldest dropped.
- [ ] Sim restart → history and last landing visible.
- [ ] Corrupt/missing file → empty history, error logged, no crash.
- [ ] Unit tests for the ring buffer and the (de)serialisation.
