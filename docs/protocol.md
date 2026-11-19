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
| `hello` | `{ "module_version": "0.1.1", "ticks": 12 }` | answer to `panel_ready` (M01 spike) |
| `tick` | `{ "count": 13 }` | once per second (M01 spike, removed in M04) |

M04 replaces the spike types with `state` (current landing + context) and `landing` (new landing completed); M07 adds `history`.
