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

## Acceptance criteria

- [ ] Landings at 3 airports (incl. one with displaced threshold) show correct runway, distance (±30 m vs. visual check) and side.
- [ ] Landing on a grass strip / off-field handled without errors.
- [ ] No frame hitch during the facility request.
