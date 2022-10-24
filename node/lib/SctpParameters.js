"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.parseSctpParametersDump = void 0;
function parseSctpParametersDump(binary) {
    return {
        port: binary.port(),
        OS: binary.os(),
        MIS: binary.mis(),
        maxMessageSize: binary.maxMessageSize(),
        sendBufferSize: binary.sendBufferSize(),
        sctpBufferedAmount: binary.sctpBufferedAmount(),
        isDataChannel: binary.isDataChannel()
    };
}
exports.parseSctpParametersDump = parseSctpParametersDump;
