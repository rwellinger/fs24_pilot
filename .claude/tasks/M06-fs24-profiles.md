# M06 — FS24 aircraft profiles

GitHub: issue #7, milestone "M06 — FS24 aircraft profiles"

## Goal

Profiles cover the FS24 default fleet, the HPG H145 and the user's add-ons; unknown aircraft get a sensible class.

## Scope

- `data/profiles.json` (format from xp_pilot `flight_logger_profiles.json`: profile classes with 4 fpm thresholds, aircraft match list). Embedded into the WASM at build time (no runtime file dependency) — decide and document.
- Matching on `ATC MODEL` (icao_type_designator), fallback `TITLE` substring; document per aircraft which value FS24 actually reports (table in `docs/research/aircraft.md`).
- Airframe classification: MSFS `ENGINE TYPE` (0 piston, 1 jet, 3 helo turbine, 5 turboprop …) + `MAX GROSS WEIGHT` (lbs → kg) + engine count, mirroring xp_pilot's rules.
- Lists: FS24 default fleet (Asobo/Microsoft), HPG H145, user's add-ons (list provided by the user in the issue).

## Out of scope

User overrides via UI (possible later milestone).

## Acceptance criteria

- [ ] Every aircraft on the three lists resolves to the intended profile (console log on load, table in `docs/research/aircraft.md`).
- [ ] An unlisted aircraft gets the classification fallback.
- [ ] Unit tests for matching and classification.
