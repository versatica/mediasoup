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
var _internal, _data, _channel, _payloadChannel, _closed, _appData, _observer;
Object.defineProperty(exports, "__esModule", { value: true });
const Logger_1 = require("./Logger");
const EnhancedEventEmitter_1 = require("./EnhancedEventEmitter");
const logger = new Logger_1.Logger('DataConsumer');
class DataConsumer extends EnhancedEventEmitter_1.EnhancedEventEmitter {
    /**
     * @private
     * @emits transportclose
     * @emits dataproducerclose
     * @emits message - (message: Buffer, ppid: number)
     * @emits sctpsendbufferfull
     * @emits bufferedamountlow - (bufferedAmount: number)
     * @emits @close
     * @emits @dataproducerclose
     */
    constructor({ internal, data, channel, payloadChannel, appData }) {
        super();
        // Internal data.
        _internal.set(this, void 0);
        // DataConsumer data.
        _data.set(this, void 0);
        // Channel instance.
        _channel.set(this, void 0);
        // PayloadChannel instance.
        _payloadChannel.set(this, void 0);
        // Closed flag.
        _closed.set(this, false);
        // Custom app data.
        _appData.set(this, void 0);
        // Observer instance.
        _observer.set(this, new EnhancedEventEmitter_1.EnhancedEventEmitter());
        logger.debug('constructor()');
        __classPrivateFieldSet(this, _internal, internal);
        __classPrivateFieldSet(this, _data, data);
        __classPrivateFieldSet(this, _channel, channel);
        __classPrivateFieldSet(this, _payloadChannel, payloadChannel);
        __classPrivateFieldSet(this, _appData, appData);
        this.handleWorkerNotifications();
    }
    /**
     * DataConsumer id.
     */
    get id() {
        return __classPrivateFieldGet(this, _internal).dataConsumerId;
    }
    /**
     * Associated DataProducer id.
     */
    get dataProducerId() {
        return __classPrivateFieldGet(this, _internal).dataProducerId;
    }
    /**
     * Whether the DataConsumer is closed.
     */
    get closed() {
        return __classPrivateFieldGet(this, _closed);
    }
    /**
     * DataConsumer type.
     */
    get type() {
        return __classPrivateFieldGet(this, _data).type;
    }
    /**
     * SCTP stream parameters.
     */
    get sctpStreamParameters() {
        return __classPrivateFieldGet(this, _data).sctpStreamParameters;
    }
    /**
     * DataChannel label.
     */
    get label() {
        return __classPrivateFieldGet(this, _data).label;
    }
    /**
     * DataChannel protocol.
     */
    get protocol() {
        return __classPrivateFieldGet(this, _data).protocol;
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
     */
    get observer() {
        return __classPrivateFieldGet(this, _observer);
    }
    /**
     * Close the DataConsumer.
     */
    close() {
        if (__classPrivateFieldGet(this, _closed))
            return;
        logger.debug('close()');
        __classPrivateFieldSet(this, _closed, true);
        // Remove notification subscriptions.
        __classPrivateFieldGet(this, _channel).removeAllListeners(__classPrivateFieldGet(this, _internal).dataConsumerId);
        __classPrivateFieldGet(this, _payloadChannel).removeAllListeners(__classPrivateFieldGet(this, _internal).dataConsumerId);
        __classPrivateFieldGet(this, _channel).request('dataConsumer.close', __classPrivateFieldGet(this, _internal))
            .catch(() => { });
        this.emit('@close');
        // Emit observer event.
        __classPrivateFieldGet(this, _observer).safeEmit('close');
    }
    /**
     * Transport was closed.
     *
     * @private
     */
    transportClosed() {
        if (__classPrivateFieldGet(this, _closed))
            return;
        logger.debug('transportClosed()');
        __classPrivateFieldSet(this, _closed, true);
        // Remove notification subscriptions.
        __classPrivateFieldGet(this, _channel).removeAllListeners(__classPrivateFieldGet(this, _internal).dataConsumerId);
        __classPrivateFieldGet(this, _payloadChannel).removeAllListeners(__classPrivateFieldGet(this, _internal).dataConsumerId);
        this.safeEmit('transportclose');
        // Emit observer event.
        __classPrivateFieldGet(this, _observer).safeEmit('close');
    }
    /**
     * Dump DataConsumer.
     */
    async dump() {
        logger.debug('dump()');
        return __classPrivateFieldGet(this, _channel).request('dataConsumer.dump', __classPrivateFieldGet(this, _internal));
    }
    /**
     * Get DataConsumer stats.
     */
    async getStats() {
        logger.debug('getStats()');
        return __classPrivateFieldGet(this, _channel).request('dataConsumer.getStats', __classPrivateFieldGet(this, _internal));
    }
    /**
     * Set buffered amount low threshold.
     */
    async setBufferedAmountLowThreshold(threshold) {
        logger.debug('setBufferedAmountLowThreshold() [threshold:%s]', threshold);
        const reqData = { threshold };
        await __classPrivateFieldGet(this, _channel).request('dataConsumer.setBufferedAmountLowThreshold', __classPrivateFieldGet(this, _internal), reqData);
    }
    /**
     * Send data.
     */
    async send(message, ppid) {
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
        const requestData = { ppid };
        await __classPrivateFieldGet(this, _payloadChannel).request('dataConsumer.send', __classPrivateFieldGet(this, _internal), requestData, message);
    }
    /**
     * Get buffered amount size.
     */
    async getBufferedAmount() {
        logger.debug('getBufferedAmount()');
        const { bufferedAmount } = await __classPrivateFieldGet(this, _channel).request('dataConsumer.getBufferedAmount', __classPrivateFieldGet(this, _internal));
        return bufferedAmount;
    }
    handleWorkerNotifications() {
        __classPrivateFieldGet(this, _channel).on(__classPrivateFieldGet(this, _internal).dataConsumerId, (event, data) => {
            switch (event) {
                case 'dataproducerclose':
                    {
                        if (__classPrivateFieldGet(this, _closed))
                            break;
                        __classPrivateFieldSet(this, _closed, true);
                        // Remove notification subscriptions.
                        __classPrivateFieldGet(this, _channel).removeAllListeners(__classPrivateFieldGet(this, _internal).dataConsumerId);
                        __classPrivateFieldGet(this, _payloadChannel).removeAllListeners(__classPrivateFieldGet(this, _internal).dataConsumerId);
                        this.emit('@dataproducerclose');
                        this.safeEmit('dataproducerclose');
                        // Emit observer event.
                        __classPrivateFieldGet(this, _observer).safeEmit('close');
                        break;
                    }
                case 'sctpsendbufferfull':
                    {
                        this.safeEmit('sctpsendbufferfull');
                        break;
                    }
                case 'bufferedamountlow':
                    {
                        const { bufferedAmount } = data;
                        this.safeEmit('bufferedamountlow', bufferedAmount);
                        break;
                    }
                default:
                    {
                        logger.error('ignoring unknown event "%s" in channel listener', event);
                    }
            }
        });
        __classPrivateFieldGet(this, _payloadChannel).on(__classPrivateFieldGet(this, _internal).dataConsumerId, (event, data, payload) => {
            switch (event) {
                case 'message':
                    {
                        if (__classPrivateFieldGet(this, _closed))
                            break;
                        const ppid = data.ppid;
                        const message = payload;
                        this.safeEmit('message', message, ppid);
                        break;
                    }
                default:
                    {
                        logger.error('ignoring unknown event "%s" in payload channel listener', event);
                    }
            }
        });
    }
}
exports.DataConsumer = DataConsumer;
_internal = new WeakMap(), _data = new WeakMap(), _channel = new WeakMap(), _payloadChannel = new WeakMap(), _closed = new WeakMap(), _appData = new WeakMap(), _observer = new WeakMap();
