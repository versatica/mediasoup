"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const randomNumber = require("random-number");
const os_1 = require("os");
const randomNumberGenerator = randomNumber.generator({
    min: 100000000,
    max: 999999999,
    integer: true
});
exports.generateRandomNumber = randomNumberGenerator;
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
 * Get IPv4 IP addresses from all network interfaces available
 */
function getIpv4IPs() {
    const ips = [];
    for (const networkInterface of Object.values(os_1.networkInterfaces())) {
        for (const info of networkInterface !== null && networkInterface !== void 0 ? networkInterface : []) {
            if (info.family === 'IPv4') {
                ips.push(info.address);
            }
        }
    }
    return ips;
}
exports.getIpv4IPs = getIpv4IPs;
/**
 * Get IPv4 and IPv6 IP addresses from all network interfaces available
 */
function getAllIPs() {
    const ips = [];
    for (const networkInterface of Object.values(os_1.networkInterfaces())) {
        for (const info of networkInterface !== null && networkInterface !== void 0 ? networkInterface : []) {
            ips.push(info.address);
        }
    }
    return ips;
}
exports.getAllIPs = getAllIPs;
function flatten(arr) {
    // @ts-ignore Typing this correctly seems impossible
    return [].concat(...arr);
}
exports.flatten = flatten;
