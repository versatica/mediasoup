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
var _data;
Object.defineProperty(exports, "__esModule", { value: true });
const uuid_1 = require("uuid");
const Logger_1 = require("./Logger");
const ortc = require("./ortc");
const Transport_1 = require("./Transport");
const Consumer_1 = require("./Consumer");
const logger = new Logger_1.Logger('PipeTransport');
class PipeTransport extends Transport_1.Transport {
    /**
     * @private
     * @emits sctpstatechange - (sctpState: SctpState)
     * @emits trace - (trace: TransportTraceEventData)
     */
    constructor(params) {
        super(params);
        // PipeTransport data.
        _data.set(this, void 0);
        logger.debug('constructor()');
        const { data } = params;
        __classPrivateFieldSet(this, _data, {
            tuple: data.tuple,
            sctpParameters: data.sctpParameters,
            sctpState: data.sctpState,
            rtx: data.rtx,
            srtpParameters: data.srtpParameters
        });
        this.handleWorkerNotifications();
    }
    /**
     * Transport tuple.
     */
    get tuple() {
        return __classPrivateFieldGet(this, _data).tuple;
    }
    /**
     * SCTP parameters.
     */
    get sctpParameters() {
        return __classPrivateFieldGet(this, _data).sctpParameters;
    }
    /**
     * SCTP state.
     */
    get sctpState() {
        return __classPrivateFieldGet(this, _data).sctpState;
    }
    /**
     * SRTP parameters.
     */
    get srtpParameters() {
        return __classPrivateFieldGet(this, _data).srtpParameters;
    }
    /**
     * Observer.
     *
     * @override
     * @emits close
     * @emits newproducer - (producer: Producer)
     * @emits newconsumer - (consumer: Consumer)
     * @emits newdataproducer - (dataProducer: DataProducer)
     * @emits newdataconsumer - (dataConsumer: DataConsumer)
     * @emits sctpstatechange - (sctpState: SctpState)
     * @emits trace - (trace: TransportTraceEventData)
     */
    // get observer(): EnhancedEventEmitter
    /**
     * Close the PipeTransport.
     *
     * @override
     */
    close() {
        if (this.closed)
            return;
        if (__classPrivateFieldGet(this, _data).sctpState)
            __classPrivateFieldGet(this, _data).sctpState = 'closed';
        super.close();
    }
    /**
     * Router was closed.
     *
     * @private
     * @override
     */
    routerClosed() {
        if (this.closed)
            return;
        if (__classPrivateFieldGet(this, _data).sctpState)
            __classPrivateFieldGet(this, _data).sctpState = 'closed';
        super.routerClosed();
    }
    /**
     * Get PipeTransport stats.
     *
     * @override
     */
    async getStats() {
        logger.debug('getStats()');
        return this.channel.request('transport.getStats', this.internal);
    }
    /**
     * Provide the PipeTransport remote parameters.
     *
     * @override
     */
    async connect({ ip, port, srtpParameters }) {
        logger.debug('connect()');
        const reqData = { ip, port, srtpParameters };
        const data = await this.channel.request('transport.connect', this.internal, reqData);
        // Update data.
        __classPrivateFieldGet(this, _data).tuple = data.tuple;
    }
    /**
     * Create a pipe Consumer.
     *
     * @override
     */
    async consume({ producerId, appData = {} }) {
        logger.debug('consume()');
        if (!producerId || typeof producerId !== 'string')
            throw new TypeError('missing producerId');
        else if (appData && typeof appData !== 'object')
            throw new TypeError('if given, appData must be an object');
        const producer = this.getProducerById(producerId);
        if (!producer)
            throw Error(`Producer with id "${producerId}" not found`);
        // This may throw.
        const rtpParameters = ortc.getPipeConsumerRtpParameters(producer.consumableRtpParameters, __classPrivateFieldGet(this, _data).rtx);
        const internal = { ...this.internal, consumerId: uuid_1.v4(), producerId };
        const reqData = {
            kind: producer.kind,
            rtpParameters,
            type: 'pipe',
            consumableRtpEncodings: producer.consumableRtpParameters.encodings
        };
        const status = await this.channel.request('transport.consume', internal, reqData);
        const data = { kind: producer.kind, rtpParameters, type: 'pipe' };
        const consumer = new Consumer_1.Consumer({
            internal,
            data,
            channel: this.channel,
            payloadChannel: this.payloadChannel,
            appData,
            paused: status.paused,
            producerPaused: status.producerPaused
        });
        this.consumers.set(consumer.id, consumer);
        consumer.on('@close', () => this.consumers.delete(consumer.id));
        consumer.on('@producerclose', () => this.consumers.delete(consumer.id));
        // Emit observer event.
        this.observer.safeEmit('newconsumer', consumer);
        return consumer;
    }
    handleWorkerNotifications() {
        this.channel.on(this.internal.transportId, (event, data) => {
            switch (event) {
                case 'sctpstatechange':
                    {
                        const sctpState = data.sctpState;
                        __classPrivateFieldGet(this, _data).sctpState = sctpState;
                        this.safeEmit('sctpstatechange', sctpState);
                        // Emit observer event.
                        this.observer.safeEmit('sctpstatechange', sctpState);
                        break;
                    }
                case 'trace':
                    {
                        const trace = data;
                        this.safeEmit('trace', trace);
                        // Emit observer event.
                        this.observer.safeEmit('trace', trace);
                        break;
                    }
                default:
                    {
                        logger.error('ignoring unknown event "%s"', event);
                    }
            }
        });
    }
}
exports.PipeTransport = PipeTransport;
_data = new WeakMap();
