/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 * Licensed under the MIT License. See the LICENSE file in the project root.
 */

// Transport layer: CommBus <-> protocol envelopes (docs/protocol.md).
// Knows event names and the protocol version, nothing about the view.
// Constants live outside the class: Coherent GT's JS engine may lack static class fields.
const FS24_PROTOCOL_VERSION = 1;
const FS24_TO_PANEL_EVENT = "fs24.pilot.to_panel";
const FS24_TO_MODULE_EVENT = "fs24.pilot.to_module";

class Fs24Transport {
    constructor(onMessage, onProtocolMismatch) {
        this.onMessage = onMessage;
        this.onProtocolMismatch = onProtocolMismatch;
        this.listener = null;
    }

    connect(onConnected) {
        this.listener = RegisterCommBusListener(() => {
            this.listener.on(FS24_TO_PANEL_EVENT, (raw) => this.receive(raw));
            onConnected();
        });
    }

    send(type, payload = {}) {
        const message = { protocol_version: FS24_PROTOCOL_VERSION, type, payload };
        this.listener.callWasm(FS24_TO_MODULE_EVENT, JSON.stringify(message));
    }

    receive(raw) {
        let message;
        try {
            message = JSON.parse(raw);
        } catch (err) {
            console.error(`[fs24-pilot] unparsable message: ${raw}`);
            return;
        }
        if (message.protocol_version !== FS24_PROTOCOL_VERSION) {
            this.onProtocolMismatch(message.protocol_version);
            return;
        }
        this.onMessage(message.type, message.payload);
    }
}
