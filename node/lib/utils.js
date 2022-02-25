"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.getTriplet = exports.generateRandomNumber = exports.clone = void 0;
const crypto_1 = require("crypto");
const os_1 = require("os");
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
 * Get the current platform triplet.
 *
 * @returns {string}
 */
function getTriplet() {
    return `${(0, os_1.platform)()}-${(0, os_1.arch)()}`;
}
exports.getTriplet = getTriplet;
