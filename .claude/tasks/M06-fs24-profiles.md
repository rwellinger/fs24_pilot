# M06 — FS24 aircraft profiles

GitHub: issue #7, milestone "M06 — FS24 aircraft profiles"

## Goal

Profiles cover the FS24 default fleet, the HPG H145 and the user's add-ons; unknown aircraft get a sensible class.

## Scope

- `data/profiles.json` (format from xp_pilot `flight_logger_profiles.json`: profile classes with 4 fpm thresholds, aircraft match list). Embedded into the WASM at build time (no runtime file dependency) — decide and document.
- Matching on `ATC MODEL`, fallback `TITLE` substring. `ATC MODEL` is the vendor's free-text model name, not the ICAO designator: observed `C172`, `DA42`, `C208`, `BE60` (ICAO-like), `KODIAK` (product name), `P46` (M500), `850` (TBM 850), `$$:PHENOM 300E` (FSReborn) and `$$:PC12`, `$$:PA28` (Asobo default fleet) — `$$:` is common, strip it. `TITLE` is logged since 0.3.1 and is the more telling match key. Match lists therefore hold the observed strings, not just ICAO codes; document per aircraft which value FS24 actually reports (table in `docs/research/aircraft.md`).
- Airframe classification: MSFS `ENGINE TYPE` (0 piston, 1 jet, 3 helo turbine, 5 turboprop …) + `MAX GROSS WEIGHT` (lbs → kg) + engine count, mirroring xp_pilot's rules.
- Lists: FS24 default fleet (Asobo/Microsoft), HPG H145, user's add-ons (list provided by the user in the issue).
- **Must have (user priority): Kodiak 2024** (Daher/SimWorks Studios Kodiak 100). Observed in M03: `ATC MODEL` reports `KODIAK` (neither the ICAO designator `KODI` nor xp_pilot's `KODK`) → match `KODIAK` and `KODI`. Classification already yields `turboprop`; keep it unless the test landings suggest otherwise.
- **Must have (user add-on): Phenom 300E** (FSReborn, ICAO `E55P`). Observed in M03: `ATC MODEL` reports the localisation key `$$:PHENOM 300E`, not the ICAO designator → strip the `$$:` prefix and match on ICAO *or* name substring. Classification already yields `vlj`; verify the thresholds against test landings.

## Observed in the sim (0.3.0, classification only, no match list yet)

Default fleet: `A20N` heavy_jet, `152` / `C152` light_ga, `C185` medium_ga, `C400TT` medium_ga, `SR22` medium_ga, `SF50` vlj, `TBM9` turboprop, `DA-40 NG` light_ga, `DA62` medium_ga, `DV20` light_ga, `HA420` vlj, `VL3` ultra_light, **`PC21` vlj (wrong: turboprop trainer, sim reports it as jet)**, `$$:PC12` turboprop, `$$:PA28` light_ga, `SIRIUS TL 3000` ultra_light, `STING S4` light_ga (check: LSA, maybe ultra_light), `BE36` medium_ga, `B350` turboprop.
Add-ons: `KODIAK` turboprop, `$$:PHENOM 300E` vlj, `P46` (M500) turboprop, `850` (TBM 850) turboprop, `BE60` (Black Square Turbine Duke) turboprop — correct, `C208` turboprop, `DA42` medium_ga.
Helicopters: pending.

## Out of scope

User overrides via UI (possible later milestone).

## Acceptance criteria

- [ ] Every aircraft on the three lists resolves to the intended profile (console log on load, table in `docs/research/aircraft.md`).
- [ ] An unlisted aircraft gets the classification fallback.
- [ ] Unit tests for matching and classification.
