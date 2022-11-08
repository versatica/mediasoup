"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.parseUint32StringVector = exports.parseUint16StringVector = exports.parseStringUint8Vector = exports.parseVector = exports.getRtpParametersType = exports.generateRandomNumber = exports.clone = void 0;
const crypto_1 = require("crypto");
const type_1 = require("./fbs/fbs/rtp-parameters/type");
/**
 * Clones the given object/array.
 */
function clone(data) {
    if (typeof data !== 'object')
        return {};
    return JSON.parse(JSON.stringify(data));
}
exports.clone = clone;
/**
 * Generates a random positive integer.
 */
function generateRandomNumber() {
    return (0, crypto_1.randomInt)(100_000_000, 999_999_999);
}
exports.generateRandomNumber = generateRandomNumber;
/**
 * Get the flatbuffers RtpParameters type of a given Producer.
 */
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
 * Parse flatbuffers vector into an array of the given type.
 */
function parseVector(binary, methodName, parseFn) {
    const array = [];
    for (let i = 0; i < binary[`${methodName}Length`](); ++i) {
        if (parseFn) {
            array.push(parseFn(binary[methodName](i)));
        }
        else {
            array.push(binary[methodName](i));
        }
    }
    return array;
}
exports.parseVector = parseVector;
/**
 * Parse flatbuffers vector of StringUint8 into the corresponding array.
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
 * Parse flatbuffers of Uint16String into the corresponding array.
 */
function parseUint16StringVector(binary, methodName) {
    const array = [];
    for (let i = 0; i < binary[`${methodName}Length`](); ++i) {
        const kv = binary[methodName](i);
        array.push({ key: kv.key(), value: kv.value() });
    }
    return array;
}
exports.parseUint16StringVector = parseUint16StringVector;
/**
 * Parse flatbuffers of Uint32String into the corresponding array.
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
