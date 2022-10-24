"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.parseMapUint32String = exports.parseMapStringUint8 = exports.parseVector = exports.getRtpParametersType = void 0;
const type_1 = require("./fbs/rtp-parameters/type");
function getRtpParametersType(producerType, pipe) {
    if (pipe) {
        return type_1.Type.PIPE;
    }
    switch (producerType) {
        case 'simple':
            return type_1.Type.SIMPLE;
        case 'simulcast':
            return type_1.Type.SIMULCAST;
        case 'svc':
            return type_1.Type.SVC;
        default:
            return type_1.Type.NONE;
    }
}
exports.getRtpParametersType = getRtpParametersType;
/**
 * Parse vector into an array of the given type.
 */
function parseVector(binary, methodName) {
    const array = [];
    for (let i = 0; i < binary[`${methodName}Length`](); ++i)
        array.push(binary[methodName](i));
    return array;
}
exports.parseVector = parseVector;
/**
 * Parse an array of StringUint8 into the corresponding object.
 */
function parseMapStringUint8(binary, methodName) {
    const map = {};
    for (let i = 0; i < binary[`${methodName}Length`](); ++i) {
        const kv = binary[methodName](i);
        map[kv.key()] = kv.value();
    }
    return map;
}
exports.parseMapStringUint8 = parseMapStringUint8;
/**
 * Parse an array of Uint32String into the corresponding object.
 */
function parseMapUint32String(binary, methodName) {
    const map = {};
    for (let i = 0; i < binary[`${methodName}Length`](); ++i) {
        const kv = binary[methodName](i);
        map[kv.key()] = kv.value();
    }
    return map;
}
exports.parseMapUint32String = parseMapUint32String;
