"use strict";
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const events_1 = require("events");
const Logger_1 = __importDefault(require("./Logger"));
class EnhancedEventEmitter extends events_1.EventEmitter {
    constructor(logger) {
        super();
        this.setMaxListeners(Infinity);
        this._logger = logger || new Logger_1.default('EnhancedEventEmitter');
    }
    safeEmit(event, ...args) {
        const numListeners = this.listenerCount(event);
        try {
            return this.emit(event, ...args);
        }
        catch (error) {
            this._logger.error('safeEmit() | event listener threw an error [event:%s]:%o', event, error);
            return Boolean(numListeners);
        }
    }
    safeEmitAsPromise(event, ...args) {
        return __awaiter(this, void 0, void 0, function* () {
            return new Promise((resolve, reject) => (this.safeEmit(event, ...args, resolve, reject)));
        });
    }
}
exports.default = EnhancedEventEmitter;
