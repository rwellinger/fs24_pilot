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

### module → panel

| type | payload | meaning |
|---|---|---|
| `hello` | `{ "module_version": "0.2.0", "ticks": 12 }` | answer to `panel_ready` (M01 spike) |
| `tick` | `{ "count": 13 }` | once per second (M01 spike, removed in M04) |
| `landing` | see below | a landing was completed; also sent after `hello` when one exists |

### `landing` payload (M03)

```json
{ "grade": "GREAT LANDING!", "grade_index": 1, "profile": "light_ga", "is_rotorcraft": false,
  "descent_fpm": -182, "g_force": 1.23, "bounce_count": 0, "float_time_s": 3.4,
  "ias_kts": 62, "ground_speed_kts": 60, "pitch_deg": 4.5, "bank_deg": 0.8, "yaw_rate_deg_s": 0.3,
  "flare": "Very good flare", "headwind_kts": 5, "crosswind_kts": -3,
  "flap_ratio": 1.00, "gear_down": true, "autopilot_engaged": false,
  "airport": "", "distance_to_surface_m": -1,
  "runway": null }
```

`runway` is `{ "ident", "distance_from_threshold_m", "centerline_offset_m", "length_m" }` once M05 places landings. `grade_index` 0 (BUTTER) … 4 (WASTED) drives the colour.

M04 replaces the spike types (`hello` keeps the version handshake); M07 adds `history`.
