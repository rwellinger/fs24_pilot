/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 * Licensed under the MIT License. See the LICENSE file in the project root.
 */

// Panel element: wires transport → view model → view. The panel's JS only runs while it is
// open, so on every open it asks the module for its state (`panel_ready`).
class Fs24PilotPanel extends TemplateElement {
    connectedCallback() {
        // Not every Coherent TemplateElement version defines it; an exception here would stop the panel silently.
        if (super.connectedCallback) {
            super.connectedCallback();
        }
        this.moduleVersion = "";
        this.view = new Fs24LandingView(this.querySelector("#Content"));
        this.transport = new Fs24Transport(
            (type, payload) => this.onMessage(type, payload),
            (version) =>
                this.view.showStatus(`protocol mismatch: module v${version}, panel v${FS24_PROTOCOL_VERSION}`),
        );
        this.transport.connect(() => {
            this.view.showStatus("asking module…");
            this.transport.send("panel_ready");
        });
    }

    onMessage(type, payload) {
        if (type === "hello") {
            this.moduleVersion = payload.module_version;
            this.view.showStatus("connected");
            this.view.showModule(this.moduleVersion, "");
        } else if (type === "landing") {
            const model = fs24LandingViewModel(payload);
            this.view.showLanding(model);
            this.view.showModule(this.moduleVersion, model.profile);
        }
    }
}

window.customElements.define("fs24-pilot-panel", Fs24PilotPanel);
checkAutoload();
