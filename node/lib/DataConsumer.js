"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.DataConsumer = void 0;
const Logger_1 = require("./Logger");
const EnhancedEventEmitter_1 = require("./EnhancedEventEmitter");
const FbsTransport = require("./fbs/transport_generated");
const FbsRequest = require("./fbs/request_generated");
const FbsDataConsumer = require("./fbs/dataConsumer_generated");
const logger = new Logger_1.Logger('DataConsumer');
class DataConsumer extends EnhancedEventEmitter_1.EnhancedEventEmitter {
    // Internal data.
    #internal;
    // DataConsumer data.
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
     */
    constructor({ internal, data, channel, payloadChannel, appData }) {
        super();
        logger.debug('constructor()');
        this.#internal = internal;
        this.#data = data;
        this.#channel = channel;
        this.#payloadChannel = payloadChannel;
        this.#appData = appData || {};
        this.handleWorkerNotifications();
    }
    /**
     * DataConsumer id.
     */
    get id() {
        return this.#internal.dataConsumerId;
    }
    /**
     * Associated DataProducer id.
     */
    get dataProducerId() {
        return this.#data.dataProducerId;
    }
    /**
     * Whether the DataConsumer is closed.
     */
    get closed() {
        return this.#closed;
    }
    /**
     * DataConsumer type.
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
     */
    get observer() {
        return this.#observer;
    }
    /**
     * Close the DataConsumer.
     */
    close() {
        if (this.#closed)
            return;
        logger.debug('close()');
        this.#closed = true;
        // Remove notification subscriptions.
        this.#channel.removeAllListeners(this.#internal.dataConsumerId);
        this.#payloadChannel.removeAllListeners(this.#internal.dataConsumerId);
        /* Build Request. */
        const builder = this.#channel.bufferBuilder;
        const requestOffset = new FbsTransport.CloseDataConsumerRequestT(this.#internal.dataConsumerId).pack(builder);
        this.#channel.requestBinary(FbsRequest.Method.TRANSPORT_CLOSE_DATA_CONSUMER, FbsRequest.Body.FBS_Transport_CloseDataConsumerRequest, requestOffset, this.#internal.transportId).catch(() => { });
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
        this.#channel.removeAllListeners(this.#internal.dataConsumerId);
        this.#payloadChannel.removeAllListeners(this.#internal.dataConsumerId);
        this.safeEmit('transportclose');
        // Emit observer event.
        this.#observer.safeEmit('close');
    }
    /**
     * Dump DataConsumer.
     */
    async dump() {
        logger.debug('dump()');
        return this.#channel.request('dataConsumer.dump', this.#internal.dataConsumerId);
    }
    /**
     * Get DataConsumer stats.
     */
    async getStats() {
        logger.debug('getStats()');
        return this.#channel.request('dataConsumer.getStats', this.#internal.dataConsumerId);
    }
    /**
     * Set buffered amount low threshold.
     */
    async setBufferedAmountLowThreshold(threshold) {
        logger.debug('setBufferedAmountLowThreshold() [threshold:%s]', threshold);
        /* Build Request. */
        const builder = this.#channel.bufferBuilder;
        const requestOffset = new FbsDataConsumer.SetBufferedAmountLowThresholdRequestT(threshold).pack(builder);
        this.#channel.requestBinary(FbsRequest.Method.DATA_CONSUMER_SET_BUFFERED_AMOUNT_LOW_THRESHOLD, FbsRequest.Body.FBS_DataConsumer_SetBufferedAmountLowThresholdRequest, requestOffset, this.#internal.dataConsumerId).catch(() => { });
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
        const requestData = String(ppid);
        await this.#payloadChannel.request('dataConsumer.send', this.#internal.dataConsumerId, requestData, message);
    }
    /**
     * Get buffered amount size.
     */
    async getBufferedAmount() {
        logger.debug('getBufferedAmount()');
        const response = await this.#channel.requestBinary(FbsRequest.Method.DATA_CONSUMER_GET_BUFFERED_AMOUNT, undefined, undefined, this.#internal.dataConsumerId);
        const data = new FbsDataConsumer.GetBufferedAmountResponse();
        response.body(data);
        return data.bufferedAmount();
    }
    handleWorkerNotifications() {
        this.#channel.on(this.#internal.dataConsumerId, (event, data) => {
            switch (event) {
                case 'dataproducerclose':
                    {
                        if (this.#closed)
                            break;
                        this.#closed = true;
                        // Remove notification subscriptions.
                        this.#channel.removeAllListeners(this.#internal.dataConsumerId);
                        this.#payloadChannel.removeAllListeners(this.#internal.dataConsumerId);
                        this.emit('@dataproducerclose');
                        this.safeEmit('dataproducerclose');
                        // Emit observer event.
                        this.#observer.safeEmit('close');
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
        this.#payloadChannel.on(this.#internal.dataConsumerId, (event, data, payload) => {
            switch (event) {
                case 'message':
                    {
                        if (this.#closed)
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
