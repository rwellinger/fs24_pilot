#!/usr/bin/env python3
# fs24_pilot - Landing rating for MSFS 2024
# Copyright (c) 2026 thWelly
# Licensed under the MIT License. See the LICENSE file in the project root.
"""Assemble the Community package without fspackagetool.

Result: out/fs24-pilot/{manifest.json, layout.json, modules/fs24-pilot.wasm, ...}
plus out/fs24-pilot-<version>.zip (unzip into the Community folder).

Format follows packages built by Asobo's Package Builder (proven in
fs24_streamdeck): layout.json lists payload files only — neither manifest.json
nor itself — and the manifest has no total_package_size.
"""

from __future__ import annotations

import json
import shutil
import sys
import zipfile
from pathlib import Path

NAME = "fs24-pilot"
ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "out"
PKG = OUT / NAME
WASM = ROOT / "wasm" / "build" / f"{NAME}.wasm"
PANEL = ROOT / "panel" / "package"
VERSION = (ROOT / "wasm" / "VERSION").read_text(encoding="utf-8").strip()

# Windows FILETIME: 100 ns ticks since 1601-01-01 UTC.
FILETIME_EPOCH_OFFSET = 116444736000000000


def to_filetime(mtime_ns: int) -> int:
    return mtime_ns // 100 + FILETIME_EPOCH_OFFSET


def manifest() -> dict:
    return {
        "dependencies": [],
        "content_type": "MISC",
        "title": "fs24 Pilot — Landing Rating",
        "manufacturer": "",
        "creator": "thWelly",
        "package_version": VERSION,
        "minimum_game_version": "1.0.0",
        "release_notes": {"neutral": {"LastUpdate": "", "OlderHistory": ""}},
    }


def copy_payload() -> None:
    if not WASM.is_file():
        sys.exit(f"missing {WASM.relative_to(ROOT)} — run 'make wasm' first")
    shutil.rmtree(PKG, ignore_errors=True)
    (PKG / "modules").mkdir(parents=True)
    shutil.copy2(WASM, PKG / "modules" / WASM.name)
    # panel/package/ is laid out exactly as the files appear in the package.
    if not any(PANEL.glob("InGamePanels/*.spb")):
        print("warning: no InGamePanels/*.spb — the toolbar panel will not show (docs/research/panel.md)")
    for src in sorted(p for p in PANEL.rglob("*") if p.is_file()):
        dest = PKG / src.relative_to(PANEL)
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dest)


def layout() -> dict:
    content = []
    for f in sorted(PKG.rglob("*")):
        if f.is_file() and f.name not in ("manifest.json", "layout.json"):
            st = f.stat()
            content.append({"path": f.relative_to(PKG).as_posix(), "size": st.st_size, "date": to_filetime(st.st_mtime_ns)})
    return {"content": content}


def write_json(path: Path, data: dict) -> None:
    path.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def zip_package() -> Path:
    archive = OUT / f"{NAME}-{VERSION}.zip"
    archive.unlink(missing_ok=True)
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as z:
        for f in sorted(PKG.rglob("*")):
            if f.is_file():
                z.write(f, f.relative_to(OUT).as_posix())
    return archive


def main() -> None:
    copy_payload()
    write_json(PKG / "layout.json", layout())
    write_json(PKG / "manifest.json", manifest())
    archive = zip_package()
    print(f"{NAME} {VERSION} -> {PKG.relative_to(ROOT)} + {archive.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
