# M04 — Panel UI

GitHub: issue #5, milestone "M04 — Panel UI"

## Goal

The toolbar panel shows the latest landing like the xp_pilot popup.

## Scope

- Protocol v1 final in `docs/protocol.md` (`landing` message: grade, metrics, context, runway block optional). `ui_bus` adapter implements `ResultPublisher`.
- Panel layers: transport (CommBus listener, protocol version check) → view model → view.
- Content (from `xp_pilot/src/ui_landing_popup.cpp`): rating banner with grade colours (BUTTER yellow, GREAT light green, ACCEPTABLE green, HARD orange, WASTED red), VS, G (coloured by own grade), float time, IAS/GS, bounces or crosswind, flare label, context line (flaps, gear not down, autoland); rotorcraft variant (VS, G, drift/run-on, bank, yaw rate).
- Runway diagram as inline SVG (strip, 300 m zone, touchdown dot, caption) — renders "no runway data" until M05.
- Behaviour per M01 findings (auto-show or docked live update).

## Out of scope

History list (M07), settings UI.

## Implementation notes (2026-11-20, pulled ahead of the remaining M03 sim tests)

- Domain `Rating { overall, CriterionGrades }`: every criterion graded on its own; `Landing::criteria` → payload `criteria` so the panel colours each metric without rating anything.
- Payload additions (protocol v1, additive): `criteria`, `crosswind_side`, `wind_speed_kts`, `run_on` (`docs/protocol.md`).
- Panel: `view_model.js` + `landing_view.js` build the DOM inside `#Content`; layout of xp_pilot's popup (banner, 3/2-column metrics, flare, context with GEAR NOT DOWN / AUTOLAND highlighted, runway SVG, footer with status, profile, version).
- `panel/preview/` + `make preview`: browser preview with sample landings from the M03 flights; screenshots via headless Brave.

- Sim check 0.4.1: renders as in the preview. Bug found: "GEAR NOT DOWN" on the FSReborn Phenom — its gear system leaves `GEAR TOTAL PCT EXTENDED` at 0 → 0.4.3 treats the gear as down when a wheel touches the runway.

- Scaling (user request, large TV): content scales with the panel width (`panel_scale.js`, all CSS sizes in em). No size/position settings: the sim remembers panel size and position per panel, and no API places a toolbar panel (`docs/research/panel.md`).

## Acceptance criteria

- [x] After a landing the panel shows all metrics within 1 s. — 0.4.1, Phenom 300E (`testdata/Screenshot 2026-11-21 220902.png`).
- [ ] Rotorcraft landing shows the rotorcraft layout.
- [ ] Protocol version mismatch shows a clear message instead of garbage.
- [x] Panel readable in VR/2D at default toolbar panel size (screenshot in the issue). — 2D verified (`testdata/`), VR not tested.
