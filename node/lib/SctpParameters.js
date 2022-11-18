"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.parseSctpStreamParameters = exports.serializeSctpStreamParameters = exports.parseSctpParametersDump = void 0;
const FbsSctpParameters = require("./fbs/sctpParameters_generated");
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
function serializeSctpStreamParameters(builder, parameters) {
    return FbsSctpParameters.SctpStreamParameters.createSctpStreamParameters(builder, parameters.streamId, parameters.ordered, typeof parameters.maxPacketLifeTime === 'number' ?
        parameters.maxPacketLifeTime :
        null, typeof parameters.maxRetransmits === 'number' ?
        parameters.maxRetransmits :
        null);
}
exports.serializeSctpStreamParameters = serializeSctpStreamParameters;
function parseSctpStreamParameters(parameters) {
    return {
        streamId: parameters.streamId(),
        ordered: parameters.ordered(),
        maxPacketLifeTime: parameters.maxPacketLifeTime() !== null ?
            parameters.maxPacketLifeTime() :
            undefined,
        maxRetransmits: parameters.maxRetransmits() !== null ?
            parameters.maxRetransmits() :
            undefined
    };
}
exports.parseSctpStreamParameters = parseSctpStreamParameters;
