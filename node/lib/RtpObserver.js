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
var _closed, _paused, _appData, _observer;
Object.defineProperty(exports, "__esModule", { value: true });
const Logger_1 = require("./Logger");
const EnhancedEventEmitter_1 = require("./EnhancedEventEmitter");
const logger = new Logger_1.Logger('RtpObserver');
class RtpObserver extends EnhancedEventEmitter_1.EnhancedEventEmitter {
    /**
     * @private
     * @interface
     * @emits routerclose
     * @emits @close
     */
    constructor({ internal, channel, payloadChannel, appData, getProducerById }) {
        super();
        // Closed flag.
        _closed.set(this, false);
        // Paused flag.
        _paused.set(this, false);
        // Custom app data.
        _appData.set(this, void 0);
        // Observer instance.
        _observer.set(this, new EnhancedEventEmitter_1.EnhancedEventEmitter());
        logger.debug('constructor()');
        this.internal = internal;
        this.channel = channel;
        this.payloadChannel = payloadChannel;
        __classPrivateFieldSet(this, _appData, appData);
        this.getProducerById = getProducerById;
    }
    /**
     * RtpObserver id.
     */
    get id() {
        return this.internal.rtpObserverId;
    }
    /**
     * Whether the RtpObserver is closed.
     */
    get closed() {
        return __classPrivateFieldGet(this, _closed);
    }
    /**
     * Whether the RtpObserver is paused.
     */
    get paused() {
        return __classPrivateFieldGet(this, _paused);
    }
    /**
     * App custom data.
     */
    get appData() {
        return __classPrivateFieldGet(this, _appData);
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
        return __classPrivateFieldGet(this, _observer);
    }
    /**
     * Close the RtpObserver.
     */
    close() {
        if (__classPrivateFieldGet(this, _closed))
            return;
        logger.debug('close()');
        __classPrivateFieldSet(this, _closed, true);
        // Remove notification subscriptions.
        this.channel.removeAllListeners(this.internal.rtpObserverId);
        this.payloadChannel.removeAllListeners(this.internal.rtpObserverId);
        this.channel.request('rtpObserver.close', this.internal)
            .catch(() => { });
        this.emit('@close');
        // Emit observer event.
        __classPrivateFieldGet(this, _observer).safeEmit('close');
    }
    /**
     * Router was closed.
     *
     * @private
     */
    routerClosed() {
        if (__classPrivateFieldGet(this, _closed))
            return;
        logger.debug('routerClosed()');
        __classPrivateFieldSet(this, _closed, true);
        // Remove notification subscriptions.
        this.channel.removeAllListeners(this.internal.rtpObserverId);
        this.payloadChannel.removeAllListeners(this.internal.rtpObserverId);
        this.safeEmit('routerclose');
        // Emit observer event.
        __classPrivateFieldGet(this, _observer).safeEmit('close');
    }
    /**
     * Pause the RtpObserver.
     */
    async pause() {
        logger.debug('pause()');
        const wasPaused = __classPrivateFieldGet(this, _paused);
        await this.channel.request('rtpObserver.pause', this.internal);
        __classPrivateFieldSet(this, _paused, true);
        // Emit observer event.
        if (!wasPaused)
            __classPrivateFieldGet(this, _observer).safeEmit('pause');
    }
    /**
     * Resume the RtpObserver.
     */
    async resume() {
        logger.debug('resume()');
        const wasPaused = __classPrivateFieldGet(this, _paused);
        await this.channel.request('rtpObserver.resume', this.internal);
        __classPrivateFieldSet(this, _paused, false);
        // Emit observer event.
        if (wasPaused)
            __classPrivateFieldGet(this, _observer).safeEmit('resume');
    }
    /**
     * Add a Producer to the RtpObserver.
     */
    async addProducer({ producerId }) {
        logger.debug('addProducer()');
        const producer = this.getProducerById(producerId);
        const reqData = { producerId };
        await this.channel.request('rtpObserver.addProducer', this.internal, reqData);
        // Emit observer event.
        __classPrivateFieldGet(this, _observer).safeEmit('addproducer', producer);
    }
    /**
     * Remove a Producer from the RtpObserver.
     */
    async removeProducer({ producerId }) {
        logger.debug('removeProducer()');
        const producer = this.getProducerById(producerId);
        const reqData = { producerId };
        await this.channel.request('rtpObserver.removeProducer', this.internal, reqData);
        // Emit observer event.
        __classPrivateFieldGet(this, _observer).safeEmit('removeproducer', producer);
    }
}
exports.RtpObserver = RtpObserver;
_closed = new WeakMap(), _paused = new WeakMap(), _appData = new WeakMap(), _observer = new WeakMap();
