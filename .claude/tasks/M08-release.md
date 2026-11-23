# M08 — Release

GitHub: issue #9, milestone "M08 — Release"

## Goal

Versioned, downloadable release that a user installs by unzipping into Community.

## Scope

- `.github/workflows/release.yml`: tag `v*` → build (as `build.yml`) → GitHub release with `fs24-pilot-<version>.zip`.
- `make release VERSION=x.y.z`: bumps `wasm/VERSION`, commits, tags, pushes (after confirmation).
- README: installation, update, uninstall, known limitations, troubleshooting (Dev Mode console).
- Release notes in the package manifest (`release_notes.neutral.LastUpdate`).

## Acceptance criteria

- [ ] M03 deferred sim checks done (FPS, pause on short final, provoked bounce — `M03-sim-adapter.md`).
- [ ] `v0.1.0` release exists with the zip.
- [ ] Fresh install on the sim PC from the release zip works (final acceptance like a normal user).
