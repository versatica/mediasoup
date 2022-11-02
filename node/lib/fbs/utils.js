"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.parseUint32StringVector = exports.parseStringUint8Vector = exports.parseVector = exports.getRtpParametersType = void 0;
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
 * Parse an vector of StringUint8 into the corresponding array.
 */
function parseStringUint8Vector(binary, methodName) {
    const array = [];
    for (let i = 0; i < binary[`${methodName}Length`](); ++i) {
        const kv = binary[methodName](i);
        array.push({ key: kv.key(), value: kv.value() });
    }
    return array;
}
exports.parseStringUint8Vector = parseStringUint8Vector;
/**
 * Parse a vector of Uint32String into the corresponding array.
 */
function parseUint32StringVector(binary, methodName) {
    const array = [];
    for (let i = 0; i < binary[`${methodName}Length`](); ++i) {
        const kv = binary[methodName](i);
        array.push({ key: kv.key(), value: kv.value() });
    }
    return array;
}
exports.parseUint32StringVector = parseUint32StringVector;
