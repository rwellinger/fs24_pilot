# M05 — Runway lookup via Facilities API

GitHub: issue #6, milestone "M05 — Runway lookup via Facilities API"

## Goal

Each landing is placed on its runway: runway ident, distance past the (displaced) threshold, centerline offset.

## Scope

- `src/adapters/facilities/`: `RunwayProvider` via SimConnect Facilities API — nearest airport (`SimConnect_RequestFacilitiesList_EX1` / nearby airports), then `AddToFacilityDefinition` AIRPORT → RUNWAY (latitude, longitude, heading, length, width, primary/secondary threshold + displaced threshold, designators).
- Asynchronous: prefetch below ~2000 ft AGL (like xp_pilot's 610 m preload), attach the result on touchdown; late result updates the stored landing and re-publishes.
- Domain `RunwayGeometry` does the placement (unchanged from M02).
- Off-field detection (> 500 m from any runway/helipad).

## Out of scope

Helipad data if the Facilities API lacks it (document).

## Implementation notes (2026-11-23)

- `adapters/facilities/facility_mapping.*` (SDK-free, host-tested): definition field list, packed `Raw*` blocks, designator idents (`09`, `27L`, `N`, `36W`), runway ends from centre + heading + length, choice of the nearby airports, `AirportAssembler` (thresholds matched to their runway by `ParentUniqueRequestId`). `surface_cache.hpp`: airport with the nearest surface.
- `adapters/facilities/simconnect_runway_provider.*`: own SimConnect connection; airport list → up to 6 airports within 10 km → facility data per airport; exceptions drop the refused request so a lookup always finishes.
- `LandingMonitor`: off-field (> 500 m), late placement with replace + re-publish, second lookup around the touchdown point when not on a known field. Port additions: `RunwayProvider::lookup_pending`, `LandingRepository::replace_latest`.
- Protocol v1 + `off_field`; panel shows `<airport> RUNWAY <ident>`, OFF-FIELD with the distance, or "At <airport>" for rotorcraft.
- Helipads: the Facilities API delivers them (`HELIPAD` child) — included, the out-of-scope note does not apply.
- Docs (2026-11 site, `…/msfs2024/retail/programming-apis/simconnect/api-reference/facilities/simconnect_addtofacilitydefinition/index.md`): runway `LATITUDE/LONGITUDE` = centre, `LENGTH` includes offset thresholds but not blast pads/overruns. **Not documented, to verify in the sim:** whether runway `HEADING` is true or magnetic (assumed true), and that the data block is tightly packed.

## Sim test procedure (0.5.0)

Dev Mode console, filter `fs24-pilot`.
1. Below 2000 ft AGL on the approach, one line per fetched airport: `runways <ICAO> (magvar x): 14/32 hdg 137.9 len 1730 thr 0/0; … n helipad(s)`. Check: idents plausible; `hdg` ≈ runway number × 10 **plus** magvar east (true heading) — if it equals the magnetic heading, the heading is magnetic and the ends are rotated; `thr` matches the chart's displaced threshold.
2. After the landing: `landing: … <ICAO> runway 14, 245 m past threshold, +1.8 m off centerline`. Compare distance with the touchdown spot seen from outside (±30 m) and the side.
3. Airports: one with a displaced threshold (e.g. LSZH 28 or KSFO 28L/R), two others of your choice, a grass strip, and one landing in a field > 500 m from any runway → `off-field (… m from …)`, panel OFF-FIELD.
4. Optional late case: land right after a reposition onto short final below 2000 ft → `placed late: …` and the panel updates.
5. FPS: WASM Debug window during the lookup (approach below 2000 ft).

## Acceptance criteria

- [ ] Landings at 3 airports (incl. one with displaced threshold) show correct runway, distance (±30 m vs. visual check) and side.
- [ ] Landing on a grass strip / off-field handled without errors.
- [ ] No frame hitch during the facility request.
