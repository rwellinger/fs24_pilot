/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 * Licensed under the MIT License. See the LICENSE file in the project root.
 */

// Interim view (M01 spike + M03 landing line): replaced by view model + landing view in M04.
// Replaced by view model + landing view in M04.
class Fs24PilotPanel extends TemplateElement {
    connectedCallback() {
        // Not every Coherent TemplateElement version defines it; an exception here would stop the panel silently.
        if (super.connectedCallback) {
            super.connectedCallback();
        }
        this.el = {
            status: this.querySelector("#Status"),
            module: this.querySelector("#Module"),
            ticks: this.querySelector("#Ticks"),
            landing: this.querySelector("#Landing"),
            log: this.querySelector("#Log"),
        };
        this.transport = new Fs24Transport(
            (type, payload) => this.onMessage(type, payload),
            (version) => this.showStatus(`protocol mismatch: module v${version}, panel v${FS24_PROTOCOL_VERSION}`),
        );
        this.transport.connect(() => {
            this.showStatus("connected, asking module…");
            this.transport.send("panel_ready");
        });
    }

    onMessage(type, payload) {
        if (type === "hello") {
            this.showStatus("module answered");
            this.el.module.textContent = `module: ${payload.module_version}`;
            this.el.ticks.textContent = `ticks: ${payload.ticks}`;
        } else if (type === "tick") {
            this.el.ticks.textContent = `ticks: ${payload.count}`;
        } else if (type === "landing") {
            this.el.landing.textContent =
                `last landing: ${payload.grade} ${payload.descent_fpm} fpm ${payload.g_force} G, ` +
                `${payload.bounce_count} bounce(s), ${payload.profile}`;
        }
        this.appendLog(type);
    }

    showStatus(text) {
        this.el.status.textContent = text;
    }

    appendLog(type) {
        const line = document.createElement("div");
        line.textContent = `${new Date().toLocaleTimeString()} ${type}`;
        this.el.log.prepend(line);
        while (this.el.log.childElementCount > 5) {
            this.el.log.lastElementChild.remove();
        }
    }
}

window.customElements.define("fs24-pilot-panel", Fs24PilotPanel);
checkAutoload();
