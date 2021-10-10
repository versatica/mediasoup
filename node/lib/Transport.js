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
var _data, _closed, _appData, _getRouterRtpCapabilities, _producers, _cnameForProducers, _nextMidForConsumers, _sctpStreamIds, _nextSctpStreamId, _observer;
Object.defineProperty(exports, "__esModule", { value: true });
const uuid_1 = require("uuid");
const Logger_1 = require("./Logger");
const EnhancedEventEmitter_1 = require("./EnhancedEventEmitter");
const utils = require("./utils");
const ortc = require("./ortc");
const Producer_1 = require("./Producer");
const Consumer_1 = require("./Consumer");
const DataProducer_1 = require("./DataProducer");
const DataConsumer_1 = require("./DataConsumer");
const logger = new Logger_1.Logger('Transport');
class Transport extends EnhancedEventEmitter_1.EnhancedEventEmitter {
    /**
     * @private
     * @interface
     * @emits routerclose
     * @emits @close
     * @emits @newproducer - (producer: Producer)
     * @emits @producerclose - (producer: Producer)
     * @emits @newdataproducer - (dataProducer: DataProducer)
     * @emits @dataproducerclose - (dataProducer: DataProducer)
     */
    constructor({ internal, data, channel, payloadChannel, appData, getRouterRtpCapabilities, getProducerById, getDataProducerById }) {
        super();
        // Transport data. This is set by the subclass.
        _data.set(this, void 0);
        // Close flag.
        _closed.set(this, false);
        // Custom app data.
        _appData.set(this, void 0);
        // Method to retrieve Router RTP capabilities.
        _getRouterRtpCapabilities.set(this, void 0);
        // Producers map.
        _producers.set(this, new Map());
        // Consumers map.
        this.consumers = new Map();
        // DataProducers map.
        this.dataProducers = new Map();
        // DataConsumers map.
        this.dataConsumers = new Map();
        // RTCP CNAME for Producers.
        _cnameForProducers.set(this, void 0);
        // Next MID for Consumers. It's converted into string when used.
        _nextMidForConsumers.set(this, 0);
        // Buffer with available SCTP stream ids.
        _sctpStreamIds.set(this, void 0);
        // Next SCTP stream id.
        _nextSctpStreamId.set(this, 0);
        // Observer instance.
        _observer.set(this, new EnhancedEventEmitter_1.EnhancedEventEmitter());
        logger.debug('constructor()');
        this.internal = internal;
        __classPrivateFieldSet(this, _data, data);
        this.channel = channel;
        this.payloadChannel = payloadChannel;
        __classPrivateFieldSet(this, _appData, appData);
        __classPrivateFieldSet(this, _getRouterRtpCapabilities, getRouterRtpCapabilities);
        this.getProducerById = getProducerById;
        this.getDataProducerById = getDataProducerById;
    }
    /**
     * Transport id.
     */
    get id() {
        return this.internal.transportId;
    }
    /**
     * Whether the Transport is closed.
     */
    get closed() {
        return __classPrivateFieldGet(this, _closed);
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
     * @emits newproducer - (producer: Producer)
     * @emits newconsumer - (producer: Producer)
     * @emits newdataproducer - (dataProducer: DataProducer)
     * @emits newdataconsumer - (dataProducer: DataProducer)
     */
    get observer() {
        return __classPrivateFieldGet(this, _observer);
    }
    /**
     * @private
     * Just for testing purposes.
     */
    get channelForTesting() {
        return this.channel;
    }
    /**
     * Close the Transport.
     */
    close() {
        if (__classPrivateFieldGet(this, _closed))
            return;
        logger.debug('close()');
        __classPrivateFieldSet(this, _closed, true);
        // Remove notification subscriptions.
        this.channel.removeAllListeners(this.internal.transportId);
        this.payloadChannel.removeAllListeners(this.internal.transportId);
        this.channel.request('transport.close', this.internal)
            .catch(() => { });
        // Close every Producer.
        for (const producer of __classPrivateFieldGet(this, _producers).values()) {
            producer.transportClosed();
            // Must tell the Router.
            this.emit('@producerclose', producer);
        }
        __classPrivateFieldGet(this, _producers).clear();
        // Close every Consumer.
        for (const consumer of this.consumers.values()) {
            consumer.transportClosed();
        }
        this.consumers.clear();
        // Close every DataProducer.
        for (const dataProducer of this.dataProducers.values()) {
            dataProducer.transportClosed();
            // Must tell the Router.
            this.emit('@dataproducerclose', dataProducer);
        }
        this.dataProducers.clear();
        // Close every DataConsumer.
        for (const dataConsumer of this.dataConsumers.values()) {
            dataConsumer.transportClosed();
        }
        this.dataConsumers.clear();
        this.emit('@close');
        // Emit observer event.
        __classPrivateFieldGet(this, _observer).safeEmit('close');
    }
    /**
     * Router was closed.
     *
     * @private
     * @virtual
     */
    routerClosed() {
        if (__classPrivateFieldGet(this, _closed))
            return;
        logger.debug('routerClosed()');
        __classPrivateFieldSet(this, _closed, true);
        // Remove notification subscriptions.
        this.channel.removeAllListeners(this.internal.transportId);
        this.payloadChannel.removeAllListeners(this.internal.transportId);
        // Close every Producer.
        for (const producer of __classPrivateFieldGet(this, _producers).values()) {
            producer.transportClosed();
            // NOTE: No need to tell the Router since it already knows (it has
            // been closed in fact).
        }
        __classPrivateFieldGet(this, _producers).clear();
        // Close every Consumer.
        for (const consumer of this.consumers.values()) {
            consumer.transportClosed();
        }
        this.consumers.clear();
        // Close every DataProducer.
        for (const dataProducer of this.dataProducers.values()) {
            dataProducer.transportClosed();
            // NOTE: No need to tell the Router since it already knows (it has
            // been closed in fact).
        }
        this.dataProducers.clear();
        // Close every DataConsumer.
        for (const dataConsumer of this.dataConsumers.values()) {
            dataConsumer.transportClosed();
        }
        this.dataConsumers.clear();
        this.safeEmit('routerclose');
        // Emit observer event.
        __classPrivateFieldGet(this, _observer).safeEmit('close');
    }
    /**
     * Dump Transport.
     */
    async dump() {
        logger.debug('dump()');
        return this.channel.request('transport.dump', this.internal);
    }
    /**
     * Get Transport stats.
     *
     * @abstract
     */
    async getStats() {
        // Should not happen.
        throw new Error('method not implemented in the subclass');
    }
    /**
     * Provide the Transport remote parameters.
     *
     * @abstract
     */
    // eslint-disable-next-line @typescript-eslint/no-unused-vars
    async connect(params) {
        // Should not happen.
        throw new Error('method not implemented in the subclass');
    }
    /**
     * Set maximum incoming bitrate for receiving media.
     */
    async setMaxIncomingBitrate(bitrate) {
        logger.debug('setMaxIncomingBitrate() [bitrate:%s]', bitrate);
        const reqData = { bitrate };
        await this.channel.request('transport.setMaxIncomingBitrate', this.internal, reqData);
    }
    /**
     * Set maximum outgoing bitrate for sending media.
     */
    async setMaxOutgoingBitrate(bitrate) {
        logger.debug('setMaxOutgoingBitrate() [bitrate:%s]', bitrate);
        const reqData = { bitrate };
        await this.channel.request('transport.setMaxOutgoingBitrate', this.internal, reqData);
    }
    /**
     * Create a Producer.
     */
    async produce({ id = undefined, kind, rtpParameters, paused = false, keyFrameRequestDelay, appData = {} }) {
        logger.debug('produce()');
        if (id && __classPrivateFieldGet(this, _producers).has(id))
            throw new TypeError(`a Producer with same id "${id}" already exists`);
        else if (!['audio', 'video'].includes(kind))
            throw new TypeError(`invalid kind "${kind}"`);
        else if (appData && typeof appData !== 'object')
            throw new TypeError('if given, appData must be an object');
        // This may throw.
        ortc.validateRtpParameters(rtpParameters);
        // If missing or empty encodings, add one.
        if (!rtpParameters.encodings ||
            !Array.isArray(rtpParameters.encodings) ||
            rtpParameters.encodings.length === 0) {
            rtpParameters.encodings = [{}];
        }
        // Don't do this in PipeTransports since there we must keep CNAME value in
        // each Producer.
        if (this.constructor.name !== 'PipeTransport') {
            // If CNAME is given and we don't have yet a CNAME for Producers in this
            // Transport, take it.
            if (!__classPrivateFieldGet(this, _cnameForProducers) && rtpParameters.rtcp && rtpParameters.rtcp.cname) {
                __classPrivateFieldSet(this, _cnameForProducers, rtpParameters.rtcp.cname);
            }
            // Otherwise if we don't have yet a CNAME for Producers and the RTP parameters
            // do not include CNAME, create a random one.
            else if (!__classPrivateFieldGet(this, _cnameForProducers)) {
                __classPrivateFieldSet(this, _cnameForProducers, uuid_1.v4().substr(0, 8));
            }
            // Override Producer's CNAME.
            rtpParameters.rtcp = rtpParameters.rtcp || {};
            rtpParameters.rtcp.cname = __classPrivateFieldGet(this, _cnameForProducers);
        }
        const routerRtpCapabilities = __classPrivateFieldGet(this, _getRouterRtpCapabilities).call(this);
        // This may throw.
        const rtpMapping = ortc.getProducerRtpParametersMapping(rtpParameters, routerRtpCapabilities);
        // This may throw.
        const consumableRtpParameters = ortc.getConsumableRtpParameters(kind, rtpParameters, routerRtpCapabilities, rtpMapping);
        const internal = { ...this.internal, producerId: id || uuid_1.v4() };
        const reqData = { kind, rtpParameters, rtpMapping, keyFrameRequestDelay, paused };
        const status = await this.channel.request('transport.produce', internal, reqData);
        const data = {
            kind,
            rtpParameters,
            type: status.type,
            consumableRtpParameters
        };
        const producer = new Producer_1.Producer({
            internal,
            data,
            channel: this.channel,
            payloadChannel: this.payloadChannel,
            appData,
            paused
        });
        __classPrivateFieldGet(this, _producers).set(producer.id, producer);
        producer.on('@close', () => {
            __classPrivateFieldGet(this, _producers).delete(producer.id);
            this.emit('@producerclose', producer);
        });
        this.emit('@newproducer', producer);
        // Emit observer event.
        __classPrivateFieldGet(this, _observer).safeEmit('newproducer', producer);
        return producer;
    }
    /**
     * Create a Consumer.
     *
     * @virtual
     */
    async consume({ producerId, rtpCapabilities, paused = false, mid, preferredLayers, pipe = false, appData = {} }) {
        var _a;
        logger.debug('consume()');
        if (!producerId || typeof producerId !== 'string')
            throw new TypeError('missing producerId');
        else if (appData && typeof appData !== 'object')
            throw new TypeError('if given, appData must be an object');
        else if (mid && (typeof mid !== 'string' || mid.length === 0))
            throw new TypeError('if given, mid must be non empty string');
        // This may throw.
        ortc.validateRtpCapabilities(rtpCapabilities);
        const producer = this.getProducerById(producerId);
        if (!producer)
            throw Error(`Producer with id "${producerId}" not found`);
        // This may throw.
        const rtpParameters = ortc.getConsumerRtpParameters(producer.consumableRtpParameters, rtpCapabilities, pipe);
        // Set MID.
        if (!pipe) {
            if (mid) {
                rtpParameters.mid = mid;
            }
            else {
                rtpParameters.mid = `${__classPrivateFieldSet(this, _nextMidForConsumers, (_a = +__classPrivateFieldGet(this, _nextMidForConsumers)) + 1), _a}`;
                // We use up to 8 bytes for MID (string).
                if (__classPrivateFieldGet(this, _nextMidForConsumers) === 100000000) {
                    logger.error(`consume() | reaching max MID value "${__classPrivateFieldGet(this, _nextMidForConsumers)}"`);
                    __classPrivateFieldSet(this, _nextMidForConsumers, 0);
                }
            }
        }
        const internal = { ...this.internal, consumerId: uuid_1.v4(), producerId };
        const reqData = {
            kind: producer.kind,
            rtpParameters,
            type: pipe ? 'pipe' : producer.type,
            consumableRtpEncodings: producer.consumableRtpParameters.encodings,
            paused,
            preferredLayers
        };
        const status = await this.channel.request('transport.consume', internal, reqData);
        const data = {
            kind: producer.kind,
            rtpParameters,
            type: pipe ? 'pipe' : producer.type
        };
        const consumer = new Consumer_1.Consumer({
            internal,
            data,
            channel: this.channel,
            payloadChannel: this.payloadChannel,
            appData,
            paused: status.paused,
            producerPaused: status.producerPaused,
            score: status.score,
            preferredLayers: status.preferredLayers
        });
        this.consumers.set(consumer.id, consumer);
        consumer.on('@close', () => this.consumers.delete(consumer.id));
        consumer.on('@producerclose', () => this.consumers.delete(consumer.id));
        // Emit observer event.
        __classPrivateFieldGet(this, _observer).safeEmit('newconsumer', consumer);
        return consumer;
    }
    /**
     * Create a DataProducer.
     */
    async produceData({ id = undefined, sctpStreamParameters, label = '', protocol = '', appData = {} } = {}) {
        logger.debug('produceData()');
        if (id && this.dataProducers.has(id))
            throw new TypeError(`a DataProducer with same id "${id}" already exists`);
        else if (appData && typeof appData !== 'object')
            throw new TypeError('if given, appData must be an object');
        let type;
        // If this is not a DirectTransport, sctpStreamParameters are required.
        if (this.constructor.name !== 'DirectTransport') {
            type = 'sctp';
            // This may throw.
            ortc.validateSctpStreamParameters(sctpStreamParameters);
        }
        // If this is a DirectTransport, sctpStreamParameters must not be given.
        else {
            type = 'direct';
            if (sctpStreamParameters) {
                logger.warn('produceData() | sctpStreamParameters are ignored when producing data on a DirectTransport');
            }
        }
        const internal = { ...this.internal, dataProducerId: id || uuid_1.v4() };
        const reqData = {
            type,
            sctpStreamParameters,
            label,
            protocol
        };
        const data = await this.channel.request('transport.produceData', internal, reqData);
        const dataProducer = new DataProducer_1.DataProducer({
            internal,
            data,
            channel: this.channel,
            payloadChannel: this.payloadChannel,
            appData
        });
        this.dataProducers.set(dataProducer.id, dataProducer);
        dataProducer.on('@close', () => {
            this.dataProducers.delete(dataProducer.id);
            this.emit('@dataproducerclose', dataProducer);
        });
        this.emit('@newdataproducer', dataProducer);
        // Emit observer event.
        __classPrivateFieldGet(this, _observer).safeEmit('newdataproducer', dataProducer);
        return dataProducer;
    }
    /**
     * Create a DataConsumer.
     */
    async consumeData({ dataProducerId, ordered, maxPacketLifeTime, maxRetransmits, appData = {} }) {
        logger.debug('consumeData()');
        if (!dataProducerId || typeof dataProducerId !== 'string')
            throw new TypeError('missing dataProducerId');
        else if (appData && typeof appData !== 'object')
            throw new TypeError('if given, appData must be an object');
        const dataProducer = this.getDataProducerById(dataProducerId);
        if (!dataProducer)
            throw Error(`DataProducer with id "${dataProducerId}" not found`);
        let type;
        let sctpStreamParameters;
        let sctpStreamId;
        // If this is not a DirectTransport, use sctpStreamParameters from the
        // DataProducer (if type 'sctp') unless they are given in method parameters.
        if (this.constructor.name !== 'DirectTransport') {
            type = 'sctp';
            sctpStreamParameters =
                utils.clone(dataProducer.sctpStreamParameters);
            // Override if given.
            if (ordered !== undefined)
                sctpStreamParameters.ordered = ordered;
            if (maxPacketLifeTime !== undefined)
                sctpStreamParameters.maxPacketLifeTime = maxPacketLifeTime;
            if (maxRetransmits !== undefined)
                sctpStreamParameters.maxRetransmits = maxRetransmits;
            // This may throw.
            sctpStreamId = this.getNextSctpStreamId();
            __classPrivateFieldGet(this, _sctpStreamIds)[sctpStreamId] = 1;
            sctpStreamParameters.streamId = sctpStreamId;
        }
        // If this is a DirectTransport, sctpStreamParameters must not be used.
        else {
            type = 'direct';
            if (ordered !== undefined ||
                maxPacketLifeTime !== undefined ||
                maxRetransmits !== undefined) {
                logger.warn('consumeData() | ordered, maxPacketLifeTime and maxRetransmits are ignored when consuming data on a DirectTransport');
            }
        }
        const { label, protocol } = dataProducer;
        const internal = { ...this.internal, dataConsumerId: uuid_1.v4(), dataProducerId };
        const reqData = {
            type,
            sctpStreamParameters,
            label,
            protocol
        };
        const data = await this.channel.request('transport.consumeData', internal, reqData);
        const dataConsumer = new DataConsumer_1.DataConsumer({
            internal,
            data,
            channel: this.channel,
            payloadChannel: this.payloadChannel,
            appData
        });
        this.dataConsumers.set(dataConsumer.id, dataConsumer);
        dataConsumer.on('@close', () => {
            this.dataConsumers.delete(dataConsumer.id);
            if (__classPrivateFieldGet(this, _sctpStreamIds))
                __classPrivateFieldGet(this, _sctpStreamIds)[sctpStreamId] = 0;
        });
        dataConsumer.on('@dataproducerclose', () => {
            this.dataConsumers.delete(dataConsumer.id);
            if (__classPrivateFieldGet(this, _sctpStreamIds))
                __classPrivateFieldGet(this, _sctpStreamIds)[sctpStreamId] = 0;
        });
        // Emit observer event.
        __classPrivateFieldGet(this, _observer).safeEmit('newdataconsumer', dataConsumer);
        return dataConsumer;
    }
    /**
     * Enable 'trace' event.
     */
    async enableTraceEvent(types = []) {
        logger.debug('pause()');
        const reqData = { types };
        await this.channel.request('transport.enableTraceEvent', this.internal, reqData);
    }
    getNextSctpStreamId() {
        if (!__classPrivateFieldGet(this, _data).sctpParameters ||
            typeof __classPrivateFieldGet(this, _data).sctpParameters.MIS !== 'number') {
            throw new TypeError('missing data.sctpParameters.MIS');
        }
        const numStreams = __classPrivateFieldGet(this, _data).sctpParameters.MIS;
        if (!__classPrivateFieldGet(this, _sctpStreamIds))
            __classPrivateFieldSet(this, _sctpStreamIds, Buffer.alloc(numStreams, 0));
        let sctpStreamId;
        for (let idx = 0; idx < __classPrivateFieldGet(this, _sctpStreamIds).length; ++idx) {
            sctpStreamId = (__classPrivateFieldGet(this, _nextSctpStreamId) + idx) % __classPrivateFieldGet(this, _sctpStreamIds).length;
            if (!__classPrivateFieldGet(this, _sctpStreamIds)[sctpStreamId]) {
                __classPrivateFieldSet(this, _nextSctpStreamId, sctpStreamId + 1);
                return sctpStreamId;
            }
        }
        throw new Error('no sctpStreamId available');
    }
}
exports.Transport = Transport;
_data = new WeakMap(), _closed = new WeakMap(), _appData = new WeakMap(), _getRouterRtpCapabilities = new WeakMap(), _producers = new WeakMap(), _cnameForProducers = new WeakMap(), _nextMidForConsumers = new WeakMap(), _sctpStreamIds = new WeakMap(), _nextSctpStreamId = new WeakMap(), _observer = new WeakMap();
