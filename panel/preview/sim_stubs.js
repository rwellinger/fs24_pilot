/*
 * Browser stand-ins for what the sim provides to a toolbar panel: TemplateElement,
 * checkAutoload and the CommBus listener. The fake module answers `panel_ready` with
 * hello + the selected sample landing, like wasm/src/module.cpp does.
 */

class TemplateElement extends HTMLElement {}

function checkAutoload() {}

const fs24Preview = {
    listener: null,
    sample: null,
    deliver(type, payload) {
        const message = { protocol_version: FS24_PROTOCOL_VERSION, type, payload };
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
            if (JSON.parse(raw).type !== "panel_ready") {
                return;
            }
            fs24Preview.deliver("hello", { module_version: "0.0.0" });
            if (fs24Preview.sample) {
                fs24Preview.deliver("landing", fs24Preview.sample);
            }
        },
    };
    fs24Preview.listener = listener;
    setTimeout(onReady, 0);
    return listener;
}
