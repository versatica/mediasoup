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
        // Buffered amount threshold.
        this._bufferedAmountLowThreshold = 0;
        // Buffered amount.
        this._bufferedAmount = 0;
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
     * Buffered amount threshold.
     */
    get bufferedAmountLowThreshold() {
        return this._bufferedAmountLowThreshold;
    }
    set bufferedAmountLowThreshold(value) {
        this._bufferedAmountLowThreshold = value;
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
                case 'bufferedamount':
                    {
                        const { bufferedAmount } = data;
                        const previousBufferedAmount = this._bufferedAmount;
                        this._bufferedAmount = bufferedAmount;
                        if (previousBufferedAmount > this._bufferedAmountLowThreshold &&
                            this._bufferedAmount <= this._bufferedAmountLowThreshold) {
                            this.safeEmit('bufferedamountlow', bufferedAmount);
                        }
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
