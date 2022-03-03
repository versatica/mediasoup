"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.generateRandomNumber = exports.clone = void 0;
const crypto_1 = require("crypto");
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
