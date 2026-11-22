# Module ↔ panel protocol

Transport: MSFS CommBus, JSON strings. Version: **1** (draft, M01).

| Direction | CommBus event | Sender API |
|---|---|---|
| module → panel | `fs24.pilot.to_panel` | `fsCommBusCall(…, FsCommBusBroadcast_JS)` |
| panel → module | `fs24.pilot.to_module` | `listener.callWasm(…)` |

Every message is an envelope:

```json
{ "protocol_version": 1, "type": "<type>", "payload": { } }
```

The receiver drops messages with another `protocol_version`; the panel shows a mismatch notice. Bump the version on any incompatible payload change; module and panel ship in the same package, so a mismatch means a stale cache or a mixed install.

## Types

### panel → module

| type | payload | meaning |
|---|---|---|
| `panel_ready` | `{}` | panel opened; module answers with its current state |
| `set_ui_scale` | `{ "percent": 150 }` | A−/A+ pressed; the module stores the text size (clamped to 50–400) |

### module → panel

| type | payload | meaning |
|---|---|---|
| `hello` | `{ "module_version": "0.4.5", "ui_scale_percent": 150 }` | answer to `panel_ready`; the panel applies the stored text size |
| `landing` | see below | a landing was completed; also sent after `hello` when one exists, including the last one before a sim restart |

### `landing` payload (M04)

```json
{ "grade": "GREAT LANDING!", "grade_index": 1, "profile": "light_ga", "is_rotorcraft": false,
  "criteria": { "descent": 1, "g_force": 0, "drift": null, "bank": null, "yaw_rate": null },
  "descent_fpm": -182, "g_force": 1.23, "bounce_count": 0, "float_time_s": 3.4,
  "ias_kts": 62, "ground_speed_kts": 60, "pitch_deg": 4.5, "bank_deg": 0.8, "yaw_rate_deg_s": 0.3,
  "flare": "Very good flare", "headwind_kts": 5, "crosswind_kts": -3, "crosswind_side": "left",
  "wind_speed_kts": 6, "run_on": false,
  "flap_ratio": 1.00, "gear_down": true, "autopilot_engaged": false,
  "airport": "", "distance_to_surface_m": -1,
  "runway": null }
```

- Grades are indices 0 (BUTTER) … 4 (WASTED); `grade_index` is the landing's grade and colours banner and border.
- `criteria` grades each criterion on its own (the landing's grade is the worst), so the panel colours each metric by the grade it earned. `drift`, `bank`, `yaw_rate` are rotorcraft only; `drift` is `null` on a run-on landing (`run_on: true`, ground speed ≥ 15 kts). Fixed-wing `descent` is graded after the crosswind allowance.
- `crosswind_kts` is positive from the right; `crosswind_side` spells it out.
- `runway` is `{ "ident", "distance_from_threshold_m", "centerline_offset_m", "length_m" }` once M05 places landings.

Fields are only ever added within protocol version 1; renaming or removing one bumps it. M07 adds `history`.
