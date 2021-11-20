"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.DataProducer = void 0;
const Logger_1 = require("./Logger");
const EnhancedEventEmitter_1 = require("./EnhancedEventEmitter");
const logger = new Logger_1.Logger('DataProducer');
class DataProducer extends EnhancedEventEmitter_1.EnhancedEventEmitter {
    // Internal data.
    #internal;
    // DataProducer data.
    #data;
    // Channel instance.
    #channel;
    // PayloadChannel instance.
    #payloadChannel;
    // Closed flag.
    #closed = false;
    // Custom app data.
    #appData;
    // Observer instance.
    #observer = new EnhancedEventEmitter_1.EnhancedEventEmitter();
    /**
     * @private
     * @emits transportclose
     * @emits @close
     */
    constructor({ internal, data, channel, payloadChannel, appData }) {
        super();
        logger.debug('constructor()');
        this.#internal = internal;
        this.#data = data;
        this.#channel = channel;
        this.#payloadChannel = payloadChannel;
        this.#appData = appData;
        this.handleWorkerNotifications();
    }
    /**
     * DataProducer id.
     */
    get id() {
        return this.#internal.dataProducerId;
    }
    /**
     * Whether the DataProducer is closed.
     */
    get closed() {
        return this.#closed;
    }
    /**
     * DataProducer type.
     */
    get type() {
        return this.#data.type;
    }
    /**
     * SCTP stream parameters.
     */
    get sctpStreamParameters() {
        return this.#data.sctpStreamParameters;
    }
    /**
     * DataChannel label.
     */
    get label() {
        return this.#data.label;
    }
    /**
     * DataChannel protocol.
     */
    get protocol() {
        return this.#data.protocol;
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
     */
    get observer() {
        return this.#observer;
    }
    /**
     * Close the DataProducer.
     */
    close() {
        if (this.#closed)
            return;
        logger.debug('close()');
        this.#closed = true;
        // Remove notification subscriptions.
        this.#channel.removeAllListeners(this.#internal.dataProducerId);
        this.#payloadChannel.removeAllListeners(this.#internal.dataProducerId);
        this.#channel.request('dataProducer.close', this.#internal)
            .catch(() => { });
        this.emit('@close');
        // Emit observer event.
        this.#observer.safeEmit('close');
    }
    /**
     * Transport was closed.
     *
     * @private
     */
    transportClosed() {
        if (this.#closed)
            return;
        logger.debug('transportClosed()');
        this.#closed = true;
        // Remove notification subscriptions.
        this.#channel.removeAllListeners(this.#internal.dataProducerId);
        this.#payloadChannel.removeAllListeners(this.#internal.dataProducerId);
        this.safeEmit('transportclose');
        // Emit observer event.
        this.#observer.safeEmit('close');
    }
    /**
     * Dump DataProducer.
     */
    async dump() {
        logger.debug('dump()');
        return this.#channel.request('dataProducer.dump', this.#internal);
    }
    /**
     * Get DataProducer stats.
     */
    async getStats() {
        logger.debug('getStats()');
        return this.#channel.request('dataProducer.getStats', this.#internal);
    }
    /**
     * Send data (just valid for DataProducers created on a DirectTransport).
     */
    send(message, ppid) {
        if (typeof message !== 'string' && !Buffer.isBuffer(message)) {
            throw new TypeError('message must be a string or a Buffer');
        }
        /*
         * +-------------------------------+----------+
         * | Value                         | SCTP     |
         * |                               | PPID     |
         * +-------------------------------+----------+
         * | WebRTC String                 | 51       |
         * | WebRTC Binary Partial         | 52       |
         * | (Deprecated)                  |          |
         * | WebRTC Binary                 | 53       |
         * | WebRTC String Partial         | 54       |
         * | (Deprecated)                  |          |
         * | WebRTC String Empty           | 56       |
         * | WebRTC Binary Empty           | 57       |
         * +-------------------------------+----------+
         */
        if (typeof ppid !== 'number') {
            ppid = (typeof message === 'string')
                ? message.length > 0 ? 51 : 56
                : message.length > 0 ? 53 : 57;
        }
        // Ensure we honor PPIDs.
        if (ppid === 56)
            message = ' ';
        else if (ppid === 57)
            message = Buffer.alloc(1);
        const notifData = { ppid };
        this.#payloadChannel.notify('dataProducer.send', this.#internal, notifData, message);
    }
    handleWorkerNotifications() {
        // No need to subscribe to any event.
    }
}
exports.DataProducer = DataProducer;
