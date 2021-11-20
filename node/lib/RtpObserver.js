"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.RtpObserver = void 0;
const Logger_1 = require("./Logger");
const EnhancedEventEmitter_1 = require("./EnhancedEventEmitter");
const logger = new Logger_1.Logger('RtpObserver');
class RtpObserver extends EnhancedEventEmitter_1.EnhancedEventEmitter {
    // Internal data.
    internal;
    // Channel instance.
    channel;
    // PayloadChannel instance.
    payloadChannel;
    // Closed flag.
    #closed = false;
    // Paused flag.
    #paused = false;
    // Custom app data.
    #appData;
    // Method to retrieve a Producer.
    getProducerById;
    // Observer instance.
    #observer = new EnhancedEventEmitter_1.EnhancedEventEmitter();
    /**
     * @private
     * @interface
     * @emits routerclose
     * @emits @close
     */
    constructor({ internal, channel, payloadChannel, appData, getProducerById }) {
        super();
        logger.debug('constructor()');
        this.internal = internal;
        this.channel = channel;
        this.payloadChannel = payloadChannel;
        this.#appData = appData;
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
        return this.#closed;
    }
    /**
     * Whether the RtpObserver is paused.
     */
    get paused() {
        return this.#paused;
    }
    /**
     * App custom data.
     */
    get appData() {
        return this.#appData;
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
        return this.#observer;
    }
    /**
     * Close the RtpObserver.
     */
    close() {
        if (this.#closed)
            return;
        logger.debug('close()');
        this.#closed = true;
        // Remove notification subscriptions.
        this.channel.removeAllListeners(this.internal.rtpObserverId);
        this.payloadChannel.removeAllListeners(this.internal.rtpObserverId);
        this.channel.request('rtpObserver.close', this.internal)
            .catch(() => { });
        this.emit('@close');
        // Emit observer event.
        this.#observer.safeEmit('close');
    }
    /**
     * Router was closed.
     *
     * @private
     */
    routerClosed() {
        if (this.#closed)
            return;
        logger.debug('routerClosed()');
        this.#closed = true;
        // Remove notification subscriptions.
        this.channel.removeAllListeners(this.internal.rtpObserverId);
        this.payloadChannel.removeAllListeners(this.internal.rtpObserverId);
        this.safeEmit('routerclose');
        // Emit observer event.
        this.#observer.safeEmit('close');
    }
    /**
     * Pause the RtpObserver.
     */
    async pause() {
        logger.debug('pause()');
        const wasPaused = this.#paused;
        await this.channel.request('rtpObserver.pause', this.internal);
        this.#paused = true;
        // Emit observer event.
        if (!wasPaused)
            this.#observer.safeEmit('pause');
    }
    /**
     * Resume the RtpObserver.
     */
    async resume() {
        logger.debug('resume()');
        const wasPaused = this.#paused;
        await this.channel.request('rtpObserver.resume', this.internal);
        this.#paused = false;
        // Emit observer event.
        if (wasPaused)
            this.#observer.safeEmit('resume');
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
        this.#observer.safeEmit('addproducer', producer);
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
        this.#observer.safeEmit('removeproducer', producer);
    }
}
exports.RtpObserver = RtpObserver;
