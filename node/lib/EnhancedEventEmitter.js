"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.EnhancedEventEmitter = void 0;
const events_1 = require("events");
const Logger_1 = require("./Logger");
const logger = new Logger_1.Logger('EnhancedEventEmitter');
class EnhancedEventEmitter extends events_1.EventEmitter {
    constructor() {
        super();
        this.setMaxListeners(Infinity);
    }
    safeEmit(event, ...args) {
        const numListeners = this.listenerCount(event);
        try {
            return this.emit(event, ...args);
        }
        catch (error) {
            logger.error('safeEmit() | event listener threw an error [event:%s]:%o', event, error);
            return Boolean(numListeners);
        }
    }
    async safeEmitAsPromise(event, ...args) {
        return new Promise((resolve, reject) => {
            try {
                this.emit(event, ...args, resolve, reject);
            }
            catch (error) {
                logger.error('safeEmitAsPromise() | event listener threw an error [event:%s]:%o', event, error);
                reject(error);
            }
        });
    }
    on(event, listener) {
        super.on(event, listener);
        return this;
    }
    off(event, listener) {
        super.off(event, listener);
        return this;
    }
    addListener(event, listener) {
        super.on(event, listener);
        return this;
    }
    prependListener(event, listener) {
        super.prependListener(event, listener);
        return this;
    }
    once(event, listener) {
        super.once(event, listener);
        return this;
    }
    prependOnceListener(event, listener) {
        super.prependOnceListener(event, listener);
        return this;
    }
    removeListener(event, listener) {
        super.off(event, listener);
        return this;
    }
}
exports.EnhancedEventEmitter = EnhancedEventEmitter;
