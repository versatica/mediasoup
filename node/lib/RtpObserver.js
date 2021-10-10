"use strict";
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
        this._closed = false;
        // Paused flag.
        this._paused = false;
        // Observer instance.
        this._observer = new EnhancedEventEmitter_1.EnhancedEventEmitter();
        logger.debug('constructor()');
        this._internal = internal;
        this._channel = channel;
        this._payloadChannel = payloadChannel;
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
        this._payloadChannel.removeAllListeners(this._internal.rtpObserverId);
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
        this._payloadChannel.removeAllListeners(this._internal.rtpObserverId);
        this.safeEmit('routerclose');
        // Emit observer event.
        this._observer.safeEmit('close');
    }
    /**
     * Pause the RtpObserver.
     */
    async pause() {
        logger.debug('pause()');
        const wasPaused = this._paused;
        await this._channel.request('rtpObserver.pause', this._internal);
        this._paused = true;
        // Emit observer event.
        if (!wasPaused)
            this._observer.safeEmit('pause');
    }
    /**
     * Resume the RtpObserver.
     */
    async resume() {
        logger.debug('resume()');
        const wasPaused = this._paused;
        await this._channel.request('rtpObserver.resume', this._internal);
        this._paused = false;
        // Emit observer event.
        if (wasPaused)
            this._observer.safeEmit('resume');
    }
    /**
     * Add a Producer to the RtpObserver.
     */
    async addProducer({ producerId }) {
        logger.debug('addProducer()');
        const producer = this._getProducerById(producerId);
        const reqData = { producerId };
        await this._channel.request('rtpObserver.addProducer', this._internal, reqData);
        // Emit observer event.
        this._observer.safeEmit('addproducer', producer);
    }
    /**
     * Remove a Producer from the RtpObserver.
     */
    async removeProducer({ producerId }) {
        logger.debug('removeProducer()');
        const producer = this._getProducerById(producerId);
        const reqData = { producerId };
        await this._channel.request('rtpObserver.removeProducer', this._internal, reqData);
        // Emit observer event.
        this._observer.safeEmit('removeproducer', producer);
    }
}
exports.RtpObserver = RtpObserver;
