# Toolbar panel + CommBus (M01)

Researched 2026-11-19 against MSFS 2024 SDK 1.8.2. Sources: DevSupport threads
[13134](https://devsupport.flightsimulator.com/t/custom-made-panels-not-generated/13134),
[17375](https://devsupport.flightsimulator.com/t/panel-not-showing-in-toolbar-despite-correct-layout-json-and-mounted-package/17375),
[14177](https://devsupport.flightsimulator.com/t/running-js-code-in-the-background/14177),
[16981](https://devsupport.flightsimulator.com/t/in-game-panels/16981),
[spb2xml24](https://devsupport.flightsimulator.com/t/release-spb2xml24-spb2xml-for-msfs-2024/18077),
the SDK JS [Communication API](https://docs.flightsimulator.com/msfs2024/retail/programming-apis/javascript/communication-api/)
and the MIT-licensed [msfs-input-viewer](https://github.com/spitice/msfs-input-viewer) (an in-game panel reported to work in 2024).

## 1. How toolbar panels are declared

- `InGamePanels/<name>.spb` in the package — compiled from a `SimBase.Document Type="InGamePanels"` XML with one `InGamePanels.InGamePanelDefinition` (`id`, `Name`, `url`, `icon`, sizes, `buttonVisible`). Source: `panel/devmode/PackageSources/InGamePanels/fs24-pilot-panel.xml`.
- The `.spb` is **mandatory**; a loose XML is ignored (17375 #7).
- HTML/JS/CSS live under `html_ui/InGamePanels/<Name>/`, the toolbar icon is `html_ui/icons/toolbar/<icon id>.svg`. Both are plain copied files — no compilation.
- Panels are fragile: any error → panel silently missing from toolbar and Coherent GT debugger. Enable it via the toolbar gear menu; the 2024 toolbar shows at most ~13 icons (17375 #2).
- Panels are hidden in career mode / challenges by design; custom missions show them again since SU4 (13134 #25). Free flight is unaffected.

## 2. Producing the `.spb` without tooling on the sim PC

- `fspackagetool.exe` drives a partial sim instance → needs MSFS 2024 installed. Not usable on CI or the Mac.
- No community XML → SPB compiler exists (only decompilers: spb2xml / spb2xml24). Writing our own needs the propdef GUIDs from the sim install and would break silently on sim updates → rejected.
- **Decision:** build the `.spb` once with the sim's own Dev Mode Project Editor (part of the sim, nothing installed) and commit it as `panel/package/InGamePanels/fs24-pilot-panel.spb`. It is our own artefact (no SDK content). It only has to be rebuilt when the panel XML changes (id, name, url, icon, default size) — HTML/JS/CSS changes don't need it.

### Rebuild procedure (sim PC, ~5 min)

1. Copy the folder `panel/devmode/` from the repo to the sim PC (any folder outside `Community/`, e.g. `Documents\fs24-pilot-devmode`).
2. MSFS 2024 → Developer Mode on → *Tools → Project Editor* → *File → Open project* → `fs24-pilot-panel-build.xml` (menu names not yet verified on 2024 — correct them here after the first build).
3. *Build All* (or Build on the package). Console should report the SPB asset group as built.
4. Take `Packages\fs24-pilot-panel-build\InGamePanels\fs24-pilot-panel.spb` from the project folder and put it into the repo at `panel/package/InGamePanels/`.
5. Delete the project folder on the sim PC again.

## 3. CommBus

- WASM: `fsCommBusRegister(name, cb, ctx)`, `fsCommBusCall(name, buf, size, FsCommBusBroadcast_JS)` (`MSFS/MSFS_CommBus.h`).
- JS: `Include` `/JS/Services/CommBus.js`, `RegisterCommBusListener(onRegistered)`, `listener.on(name, cb)`, `listener.callWasm(name, json)`.
- SimConnect clients can join (`SimConnect_SubscribeToCommBusEvent`) → a remote test client on the Mac could observe the traffic (not needed so far).
- Event names and envelope: `docs/protocol.md`.

## 4. Lifetime of the panel JS — consequence for the design

Toolbar panel JS is **killed when the panel is closed** (14177 #2). It cannot receive anything while closed, and no supported API opens a toolbar panel from WASM/JS outside missions (`ShowOnEvent` works only via mission flow events). Code injection into the toolbar (parallel42 toolbar-interop) is a hack and rejected.

Design consequence (fits the architecture anyway):
- The WASM module owns all state (current landing, history) and keeps running.
- The panel is a pure view: on open it sends `panel_ready`, the module answers with the current state; while open it receives live updates.
- No auto-open on touchdown. The user keeps the panel open (docked) or opens it after landing. An EFB app would run in the background, but only in aircraft with the 2024 EFB — possible later milestone, not needed for the rating itself.

## 5. Spike verification (sim PC)

To be filled in after the sim test:

- [ ] Toolbar icon visible (gear menu → enable fs24 Pilot), panel opens.
- [ ] Panel shows `module answered`, module version, ticks counting up every second.
- [ ] Console shows `[fs24-pilot] panel -> module: panel_ready` on every panel open.
- [ ] Close + reopen the panel: ticks continue from the module's count (state lives in WASM).
- [ ] Message size: not limited in practice for our payloads (< 4 KB) — to be confirmed in M04 with a full landing.
