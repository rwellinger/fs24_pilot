# M01 — Spike: toolbar panel + CommBus (go/no-go gate)

GitHub: issue #2, milestone "M01 — Spike: toolbar panel + CommBus (go/no-go gate)"

## Goal

Prove the UI path before porting logic: a toolbar entry opens our panel, and the WASM module can push a message that the panel displays. Main project risk (R1/R2 in `docs/research/feasibility.md`).

## Open questions — resolve first, document in `docs/research/panel.md`

1. How are toolbar panels declared in MSFS 2024 (`InGamePanels/*.xml` → `.spb`)? Is the `.spb` still mandatory?
2. Can the `.spb` be produced without `fspackagetool` on the sim PC (script / community generator)? Fallback: produce once, commit the binary (own artefact, no SDK content) — document how to regenerate.
3. CommBus WASM → JS: `fsCommBusCall(name, json, size, FsCommBusBroadcast_JS)` and `RegisterCommBusListener` in the panel. Does the panel receive messages only while open? Message size limits?
4. Can the module (or the panel itself) open/show the panel on demand (touchdown)? If not: docked panel that stays open, document the UX.
5. Panel → WASM direction (e.g. "request history"): works via CommBus? Needed for M07.

## Scope

- `panel/` laid out exactly as in the package (`html_ui/InGamePanels/FS24Pilot/…`, `InGamePanels/…`), toolbar icon.
- Panel shows a "hello" message plus a counter pushed by the module every few seconds (spike code, removed in M04).
- `docs/protocol.md` draft: envelope `{ "protocol_version": 1, "type": "...", "payload": {...} }`.

## Out of scope

Landing logic, final design.

## Acceptance criteria

- [ ] Toolbar shows the fs24 Pilot icon; clicking opens the panel.
- [ ] Panel displays messages sent by the WASM module.
- [ ] Answers to all open questions in `docs/research/panel.md`.
- [ ] Build stays reproducible from CI without tools on the sim PC (or the committed-binary fallback is documented).
- [ ] **Gate decision** recorded in the issue: go / re-decide (EFB app or other).
