"use strict";
var __classPrivateFieldSet = (this && this.__classPrivateFieldSet) || function (receiver, privateMap, value) {
    if (!privateMap.has(receiver)) {
        throw new TypeError("attempted to set private field on non-instance");
    }
    privateMap.set(receiver, value);
    return value;
};
var __classPrivateFieldGet = (this && this.__classPrivateFieldGet) || function (receiver, privateMap) {
    if (!privateMap.has(receiver)) {
        throw new TypeError("attempted to get private field on non-instance");
    }
    return privateMap.get(receiver);
};
var _debug, _warn, _error;
Object.defineProperty(exports, "__esModule", { value: true });
const debug_1 = require("debug");
const APP_NAME = 'mediasoup';
class Logger {
    constructor(prefix) {
        _debug.set(this, void 0);
        _warn.set(this, void 0);
        _error.set(this, void 0);
        if (prefix) {
            __classPrivateFieldSet(this, _debug, debug_1.default(`${APP_NAME}:${prefix}`));
            __classPrivateFieldSet(this, _warn, debug_1.default(`${APP_NAME}:WARN:${prefix}`));
            __classPrivateFieldSet(this, _error, debug_1.default(`${APP_NAME}:ERROR:${prefix}`));
        }
        else {
            __classPrivateFieldSet(this, _debug, debug_1.default(APP_NAME));
            __classPrivateFieldSet(this, _warn, debug_1.default(`${APP_NAME}:WARN`));
            __classPrivateFieldSet(this, _error, debug_1.default(`${APP_NAME}:ERROR`));
        }
        /* eslint-disable no-console */
        __classPrivateFieldGet(this, _debug).log = console.info.bind(console);
        __classPrivateFieldGet(this, _warn).log = console.warn.bind(console);
        __classPrivateFieldGet(this, _error).log = console.error.bind(console);
        /* eslint-enable no-console */
    }
    get debug() {
        return __classPrivateFieldGet(this, _debug);
    }
    get warn() {
        return __classPrivateFieldGet(this, _warn);
    }
    get error() {
        return __classPrivateFieldGet(this, _error);
    }
}
exports.Logger = Logger;
_debug = new WeakMap(), _warn = new WeakMap(), _error = new WeakMap();
