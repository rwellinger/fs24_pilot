# fs24_pilot

Landing rating for Microsoft Flight Simulator 2024 — a single Community package (WASM module + in-game toolbar panel), no external app.

Status: early development, see the GitHub milestones.

## Install

1. Download the `fs24-pilot` artifact/release zip.
2. Unzip it into your Community folder — it contains the `fs24-pilot` folder (MS Store: `%LOCALAPPDATA%\Packages\Microsoft.Limitless_8wekyb3d8bbwe\LocalCache\Packages\Community`).
3. Start MSFS 2024.

## Build

```bash
make sdk        # extract the MSFS 2024 SDK subset (needs: brew install msitools llvm lld)
make check      # format, tidy, unit tests (host)
make package    # WASM compile check + package in out/ (not loadable when built on the Mac)
make gh-build   # loadable build on the GitHub Windows runner
```

See `CLAUDE.md` and `docs/` for architecture and details.

## License

MIT. Catch2 (tests only) is BSL-1.0.
