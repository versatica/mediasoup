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
const Logger_1 = __importDefault(require("./Logger"));
const EnhancedEventEmitter_1 = __importDefault(require("./EnhancedEventEmitter"));
const logger = new Logger_1.default('RtpObserver');
class RtpObserver extends EnhancedEventEmitter_1.default {
    /**
     * @private
     * @interface
     * @emits routerclose
     * @emits @close
     */
    constructor({ internal, channel, appData, getProducerById }) {
        super(logger);
        // Closed flag.
        this._closed = false;
        // Paused flag.
        this._paused = false;
        // Observer instance.
        this._observer = new EnhancedEventEmitter_1.default();
        logger.debug('constructor()');
        this._internal = internal;
        this._channel = channel;
        this._appData = appData;
        this._getProducerById = getProducerById;
    }
    /**
     * RtpObserver id.
     */
    get id() {
        return this._internal.rtpObserverId;
    }
    /**
     * Whether the RtpObserver is closed.
     */
    get closed() {
        return this._closed;
    }
    /**
     * Whether the RtpObserver is paused.
     */
    get paused() {
        return this._paused;
    }
    /**
     * App custom data.
     */
    get appData() {
        return this._appData;
    }
    /**
     * Invalid setter.
     */
    set appData(appData) {
        throw new Error('cannot override appData object');
    }
    /**
     * Observer.
     *
     * @emits close
     * @emits pause
     * @emits resume
     * @emits addproducer - (producer: Producer)
     * @emits removeproducer - (producer: Producer)
     */
    get observer() {
        return this._observer;
    }
    /**
     * Close the RtpObserver.
     */
    close() {
        if (this._closed)
            return;
        logger.debug('close()');
        this._closed = true;
        // Remove notification subscriptions.
        this._channel.removeAllListeners(this._internal.rtpObserverId);
        this._channel.request('rtpObserver.close', this._internal)
            .catch(() => { });
        this.emit('@close');
        // Emit observer event.
        this._observer.safeEmit('close');
    }
    /**
     * Router was closed.
     *
     * @private
     */
    routerClosed() {
        if (this._closed)
            return;
        logger.debug('routerClosed()');
        this._closed = true;
        // Remove notification subscriptions.
        this._channel.removeAllListeners(this._internal.rtpObserverId);
        this.safeEmit('routerclose');
        // Emit observer event.
        this._observer.safeEmit('close');
    }
    /**
     * Pause the RtpObserver.
     */
    pause() {
        return __awaiter(this, void 0, void 0, function* () {
            logger.debug('pause()');
            const wasPaused = this._paused;
            yield this._channel.request('rtpObserver.pause', this._internal);
            this._paused = true;
            // Emit observer event.
            if (!wasPaused)
                this._observer.safeEmit('pause');
        });
    }
    /**
     * Resume the RtpObserver.
     */
    resume() {
        return __awaiter(this, void 0, void 0, function* () {
            logger.debug('resume()');
            const wasPaused = this._paused;
            yield this._channel.request('rtpObserver.resume', this._internal);
            this._paused = false;
            // Emit observer event.
            if (wasPaused)
                this._observer.safeEmit('resume');
        });
    }
    /**
     * Add a Producer to the RtpObserver.
     */
    addProducer({ producerId }) {
        return __awaiter(this, void 0, void 0, function* () {
            logger.debug('addProducer()');
            const producer = this._getProducerById(producerId);
            const internal = Object.assign(Object.assign({}, this._internal), { producerId });
            yield this._channel.request('rtpObserver.addProducer', internal);
            // Emit observer event.
            this._observer.safeEmit('addproducer', producer);
        });
    }
    /**
     * Remove a Producer from the RtpObserver.
     */
    removeProducer({ producerId }) {
        return __awaiter(this, void 0, void 0, function* () {
            logger.debug('removeProducer()');
            const producer = this._getProducerById(producerId);
            const internal = Object.assign(Object.assign({}, this._internal), { producerId });
            yield this._channel.request('rtpObserver.removeProducer', internal);
            // Emit observer event.
            this._observer.safeEmit('removeproducer', producer);
        });
    }
}
exports.default = RtpObserver;
