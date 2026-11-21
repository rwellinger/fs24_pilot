/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 * Licensed under the MIT License. See the LICENSE file in the project root.
 */

// Scales the panel content with the panel's width: every size in FS24Pilot.css is in em of
// #Content, so setting its font size here resizes everything. The user sizes the panel by
// dragging it, and the sim remembers that size per panel — no setting needed.

const FS24_WIDTH_IN_EM = 26; // the content is laid out for this many em across
const FS24_MIN_FONT_PX = 9;
const FS24_RESIZE_POLL_MS = 500; // Coherent GT may not fire `resize` for panel drags

class Fs24PanelScale {
    constructor(content) {
        this.content = content;
        this.lastWidth = -1;
        this.apply = this.apply.bind(this);
        window.addEventListener("resize", this.apply);
        this.poll = setInterval(this.apply, FS24_RESIZE_POLL_MS);
        this.apply();
    }

    apply() {
        const width = this.content.clientWidth;
        if (width <= 0 || width === this.lastWidth) {
            return;
        }
        this.lastWidth = width;
        this.content.style.fontSize = `${Math.max(FS24_MIN_FONT_PX, width / FS24_WIDTH_IN_EM)}px`;
    }

    stop() {
        window.removeEventListener("resize", this.apply);
        clearInterval(this.poll);
    }
}
