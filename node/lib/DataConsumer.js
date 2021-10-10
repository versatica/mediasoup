"use strict";
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
        // Closed flag.
        this._closed = false;
        // Observer instance.
        this._observer = new EnhancedEventEmitter_1.EnhancedEventEmitter();
        logger.debug('constructor()');
        this._internal = internal;
        this._data = data;
        this._channel = channel;
        this._payloadChannel = payloadChannel;
        this._appData = appData;
        this._handleWorkerNotifications();
    }
    /**
     * DataConsumer id.
     */
    get id() {
        return this._internal.dataConsumerId;
    }
    /**
     * Associated DataProducer id.
     */
    get dataProducerId() {
        return this._internal.dataProducerId;
    }
    /**
     * Whether the DataConsumer is closed.
     */
    get closed() {
        return this._closed;
    }
    /**
     * DataConsumer type.
     */
    get type() {
        return this._data.type;
    }
    /**
     * SCTP stream parameters.
     */
    get sctpStreamParameters() {
        return this._data.sctpStreamParameters;
    }
    /**
     * DataChannel label.
     */
    get label() {
        return this._data.label;
    }
    /**
     * DataChannel protocol.
     */
    get protocol() {
        return this._data.protocol;
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
     */
    get observer() {
        return this._observer;
    }
    /**
     * Close the DataConsumer.
     */
    close() {
        if (this._closed)
            return;
        logger.debug('close()');
        this._closed = true;
        // Remove notification subscriptions.
        this._channel.removeAllListeners(this._internal.dataConsumerId);
        this._payloadChannel.removeAllListeners(this._internal.dataConsumerId);
        this._channel.request('dataConsumer.close', this._internal)
            .catch(() => { });
        this.emit('@close');
        // Emit observer event.
        this._observer.safeEmit('close');
    }
    /**
     * Transport was closed.
     *
     * @private
     */
    transportClosed() {
        if (this._closed)
            return;
        logger.debug('transportClosed()');
        this._closed = true;
        // Remove notification subscriptions.
        this._channel.removeAllListeners(this._internal.dataConsumerId);
        this._payloadChannel.removeAllListeners(this._internal.dataConsumerId);
        this.safeEmit('transportclose');
        // Emit observer event.
        this._observer.safeEmit('close');
    }
    /**
     * Dump DataConsumer.
     */
    async dump() {
        logger.debug('dump()');
        return this._channel.request('dataConsumer.dump', this._internal);
    }
    /**
     * Get DataConsumer stats.
     */
    async getStats() {
        logger.debug('getStats()');
        return this._channel.request('dataConsumer.getStats', this._internal);
    }
    /**
     * Set buffered amount low threshold.
     */
    async setBufferedAmountLowThreshold(threshold) {
        logger.debug('setBufferedAmountLowThreshold() [threshold:%s]', threshold);
        const reqData = { threshold };
        await this._channel.request('dataConsumer.setBufferedAmountLowThreshold', this._internal, reqData);
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
        await this._payloadChannel.request('dataConsumer.send', this._internal, requestData, message);
    }
    /**
     * Get buffered amount size.
     */
    async getBufferedAmount() {
        logger.debug('getBufferedAmount()');
        const { bufferedAmount } = await this._channel.request('dataConsumer.getBufferedAmount', this._internal);
        return bufferedAmount;
    }
    _handleWorkerNotifications() {
        this._channel.on(this._internal.dataConsumerId, (event, data) => {
            switch (event) {
                case 'dataproducerclose':
                    {
                        if (this._closed)
                            break;
                        this._closed = true;
                        // Remove notification subscriptions.
                        this._channel.removeAllListeners(this._internal.dataConsumerId);
                        this._payloadChannel.removeAllListeners(this._internal.dataConsumerId);
                        this.emit('@dataproducerclose');
                        this.safeEmit('dataproducerclose');
                        // Emit observer event.
                        this._observer.safeEmit('close');
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
        this._payloadChannel.on(this._internal.dataConsumerId, (event, data, payload) => {
            switch (event) {
                case 'message':
                    {
                        if (this._closed)
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
