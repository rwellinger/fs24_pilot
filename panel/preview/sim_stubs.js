/*
 * Browser stand-ins for what the sim provides to a toolbar panel: TemplateElement,
 * checkAutoload and the CommBus listener. The fake module answers `panel_ready` with
 * hello (with the stored text size) + the selected sample landing, like wasm/src/module.cpp does.
 */

class TemplateElement extends HTMLElement {}

function checkAutoload() {}

const fs24Preview = {
    listener: null,
    sample: null,
    scalePercent: 100,
    protocolVersion: null, // null: the panel's own; resolved on send because transport.js loads after this file
    deliver(type, payload) {
        const version = this.protocolVersion === null ? FS24_PROTOCOL_VERSION : this.protocolVersion;
        const message = { protocol_version: version, type, payload };
        this.listener.handlers[FS24_TO_PANEL_EVENT](JSON.stringify(message));
    },
};

function RegisterCommBusListener(onReady) {
    const listener = {
        handlers: {},
        on(event, handler) {
            this.handlers[event] = handler;
        },
        callWasm(event, raw) {
            const message = JSON.parse(raw);
            if (message.type === "set_ui_scale") {
                fs24Preview.scalePercent = message.payload.percent;
            }
            if (message.type !== "panel_ready") {
                return;
            }
            fs24Preview.deliver("hello", { module_version: "0.0.0", ui_scale_percent: fs24Preview.scalePercent });
            if (fs24Preview.sample) {
                fs24Preview.deliver("landing", fs24Preview.sample);
            }
        },
    };
    fs24Preview.listener = listener;
    setTimeout(onReady, 0);
    return listener;
}
