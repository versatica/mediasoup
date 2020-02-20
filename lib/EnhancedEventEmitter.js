"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
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
        return new Promise((resolve, reject) => (this.safeEmit(event, ...args, resolve, reject)));
    }
}
exports.EnhancedEventEmitter = EnhancedEventEmitter;
