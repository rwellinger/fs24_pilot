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

## Acceptance criteria

- [ ] After a landing the panel shows all metrics within 1 s.
- [ ] Rotorcraft landing shows the rotorcraft layout.
- [ ] Protocol version mismatch shows a clear message instead of garbage.
- [ ] Panel readable in VR/2D at default toolbar panel size (screenshot in the issue).
