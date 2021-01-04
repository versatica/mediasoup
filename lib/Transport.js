"use strict";
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
        // Close flag.
        this._closed = false;
        // Producers map.
        this._producers = new Map();
        // Consumers map.
        this._consumers = new Map();
        // DataProducers map.
        this._dataProducers = new Map();
        // DataConsumers map.
        this._dataConsumers = new Map();
        // Next MID for Consumers. It's converted into string when used.
        this._nextMidForConsumers = 0;
        // Next SCTP stream id.
        this._nextSctpStreamId = 0;
        // Observer instance.
        this._observer = new EnhancedEventEmitter_1.EnhancedEventEmitter();
        logger.debug('constructor()');
        this._internal = internal;
        this._data = data;
        this._channel = channel;
        this._payloadChannel = payloadChannel;
        this._appData = appData;
        this._getRouterRtpCapabilities = getRouterRtpCapabilities;
        this._getProducerById = getProducerById;
        this._getDataProducerById = getDataProducerById;
    }
    /**
     * Transport id.
     */
    get id() {
        return this._internal.transportId;
    }
    /**
     * Whether the Transport is closed.
     */
    get closed() {
        return this._closed;
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
     * @emits newproducer - (producer: Producer)
     * @emits newconsumer - (producer: Producer)
     * @emits newdataproducer - (dataProducer: DataProducer)
     * @emits newdataconsumer - (dataProducer: DataProducer)
     */
    get observer() {
        return this._observer;
    }
    /**
     * Close the Transport.
     */
    close() {
        if (this._closed)
            return;
        logger.debug('close()');
        this._closed = true;
        // Remove notification subscriptions.
        this._channel.removeAllListeners(this._internal.transportId);
        this._payloadChannel.removeAllListeners(this._internal.transportId);
        this._channel.request('transport.close', this._internal)
            .catch(() => { });
        // Close every Producer.
        for (const producer of this._producers.values()) {
            producer.transportClosed();
            // Must tell the Router.
            this.emit('@producerclose', producer);
        }
        this._producers.clear();
        // Close every Consumer.
        for (const consumer of this._consumers.values()) {
            consumer.transportClosed();
        }
        this._consumers.clear();
        // Close every DataProducer.
        for (const dataProducer of this._dataProducers.values()) {
            dataProducer.transportClosed();
            // Must tell the Router.
            this.emit('@dataproducerclose', dataProducer);
        }
        this._dataProducers.clear();
        // Close every DataConsumer.
        for (const dataConsumer of this._dataConsumers.values()) {
            dataConsumer.transportClosed();
        }
        this._dataConsumers.clear();
        this.emit('@close');
        // Emit observer event.
        this._observer.safeEmit('close');
    }
    /**
     * Router was closed.
     *
     * @private
     * @virtual
     */
    routerClosed() {
        if (this._closed)
            return;
        logger.debug('routerClosed()');
        this._closed = true;
        // Remove notification subscriptions.
        this._channel.removeAllListeners(this._internal.transportId);
        this._payloadChannel.removeAllListeners(this._internal.transportId);
        // Close every Producer.
        for (const producer of this._producers.values()) {
            producer.transportClosed();
            // NOTE: No need to tell the Router since it already knows (it has
            // been closed in fact).
        }
        this._producers.clear();
        // Close every Consumer.
        for (const consumer of this._consumers.values()) {
            consumer.transportClosed();
        }
        this._consumers.clear();
        // Close every DataProducer.
        for (const dataProducer of this._dataProducers.values()) {
            dataProducer.transportClosed();
            // NOTE: No need to tell the Router since it already knows (it has
            // been closed in fact).
        }
        this._dataProducers.clear();
        // Close every DataConsumer.
        for (const dataConsumer of this._dataConsumers.values()) {
            dataConsumer.transportClosed();
        }
        this._dataConsumers.clear();
        this.safeEmit('routerclose');
        // Emit observer event.
        this._observer.safeEmit('close');
    }
    /**
     * Dump Transport.
     */
    async dump() {
        logger.debug('dump()');
        return this._channel.request('transport.dump', this._internal);
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
        await this._channel.request('transport.setMaxIncomingBitrate', this._internal, reqData);
    }
    /**
     * Create a Producer.
     */
    async produce({ id = undefined, kind, rtpParameters, paused = false, keyFrameRequestDelay, appData = {} }) {
        logger.debug('produce()');
        if (id && this._producers.has(id))
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
            if (!this._cnameForProducers && rtpParameters.rtcp && rtpParameters.rtcp.cname) {
                this._cnameForProducers = rtpParameters.rtcp.cname;
            }
            // Otherwise if we don't have yet a CNAME for Producers and the RTP parameters
            // do not include CNAME, create a random one.
            else if (!this._cnameForProducers) {
                this._cnameForProducers = uuid_1.v4().substr(0, 8);
            }
            // Override Producer's CNAME.
            rtpParameters.rtcp = rtpParameters.rtcp || {};
            rtpParameters.rtcp.cname = this._cnameForProducers;
        }
        const routerRtpCapabilities = this._getRouterRtpCapabilities();
        // This may throw.
        const rtpMapping = ortc.getProducerRtpParametersMapping(rtpParameters, routerRtpCapabilities);
        // This may throw.
        const consumableRtpParameters = ortc.getConsumableRtpParameters(kind, rtpParameters, routerRtpCapabilities, rtpMapping);
        const internal = { ...this._internal, producerId: id || uuid_1.v4() };
        const reqData = { kind, rtpParameters, rtpMapping, keyFrameRequestDelay, paused };
        const status = await this._channel.request('transport.produce', internal, reqData);
        const data = {
            kind,
            rtpParameters,
            type: status.type,
            consumableRtpParameters
        };
        const producer = new Producer_1.Producer({
            internal,
            data,
            channel: this._channel,
            payloadChannel: this._payloadChannel,
            appData,
            paused
        });
        this._producers.set(producer.id, producer);
        producer.on('@close', () => {
            this._producers.delete(producer.id);
            this.emit('@producerclose', producer);
        });
        this.emit('@newproducer', producer);
        // Emit observer event.
        this._observer.safeEmit('newproducer', producer);
        return producer;
    }
    /**
     * Create a Consumer.
     *
     * @virtual
     */
    async consume({ producerId, rtpCapabilities, paused = false, preferredLayers, pipe = false, appData = {} }) {
        logger.debug('consume()');
        if (!producerId || typeof producerId !== 'string')
            throw new TypeError('missing producerId');
        else if (appData && typeof appData !== 'object')
            throw new TypeError('if given, appData must be an object');
        // This may throw.
        ortc.validateRtpCapabilities(rtpCapabilities);
        const producer = this._getProducerById(producerId);
        if (!producer)
            throw Error(`Producer with id "${producerId}" not found`);
        // This may throw.
        const rtpParameters = ortc.getConsumerRtpParameters(producer.consumableRtpParameters, rtpCapabilities, pipe);
        // Set MID.
        if (!pipe) {
            rtpParameters.mid = `${this._nextMidForConsumers++}`;
            // We use up to 8 bytes for MID (string).
            if (this._nextMidForConsumers === 100000000) {
                logger.error(`consume() | reaching max MID value "${this._nextMidForConsumers}"`);
                this._nextMidForConsumers = 0;
            }
        }
        const internal = { ...this._internal, consumerId: uuid_1.v4(), producerId };
        const reqData = {
            kind: producer.kind,
            rtpParameters,
            type: pipe ? 'pipe' : producer.type,
            consumableRtpEncodings: producer.consumableRtpParameters.encodings,
            paused,
            preferredLayers
        };
        const status = await this._channel.request('transport.consume', internal, reqData);
        const data = {
            kind: producer.kind,
            rtpParameters,
            type: pipe ? 'pipe' : producer.type
        };
        const consumer = new Consumer_1.Consumer({
            internal,
            data,
            channel: this._channel,
            payloadChannel: this._payloadChannel,
            appData,
            paused: status.paused,
            producerPaused: status.producerPaused,
            score: status.score,
            preferredLayers: status.preferredLayers
        });
        this._consumers.set(consumer.id, consumer);
        consumer.on('@close', () => this._consumers.delete(consumer.id));
        consumer.on('@producerclose', () => this._consumers.delete(consumer.id));
        // Emit observer event.
        this._observer.safeEmit('newconsumer', consumer);
        return consumer;
    }
    /**
     * Create a DataProducer.
     */
    async produceData({ id = undefined, sctpStreamParameters, label = '', protocol = '', appData = {} } = {}) {
        logger.debug('produceData()');
        if (id && this._dataProducers.has(id))
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
        const internal = { ...this._internal, dataProducerId: id || uuid_1.v4() };
        const reqData = {
            type,
            sctpStreamParameters,
            label,
            protocol
        };
        const data = await this._channel.request('transport.produceData', internal, reqData);
        const dataProducer = new DataProducer_1.DataProducer({
            internal,
            data,
            channel: this._channel,
            payloadChannel: this._payloadChannel,
            appData
        });
        this._dataProducers.set(dataProducer.id, dataProducer);
        dataProducer.on('@close', () => {
            this._dataProducers.delete(dataProducer.id);
            this.emit('@dataproducerclose', dataProducer);
        });
        this.emit('@newdataproducer', dataProducer);
        // Emit observer event.
        this._observer.safeEmit('newdataproducer', dataProducer);
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
        const dataProducer = this._getDataProducerById(dataProducerId);
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
            sctpStreamId = this._getNextSctpStreamId();
            this._sctpStreamIds[sctpStreamId] = 1;
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
        const internal = { ...this._internal, dataConsumerId: uuid_1.v4(), dataProducerId };
        const reqData = {
            type,
            sctpStreamParameters,
            label,
            protocol
        };
        const data = await this._channel.request('transport.consumeData', internal, reqData);
        const dataConsumer = new DataConsumer_1.DataConsumer({
            internal,
            data,
            channel: this._channel,
            payloadChannel: this._payloadChannel,
            appData
        });
        this._dataConsumers.set(dataConsumer.id, dataConsumer);
        dataConsumer.on('@close', () => {
            this._dataConsumers.delete(dataConsumer.id);
            if (this._sctpStreamIds)
                this._sctpStreamIds[sctpStreamId] = 0;
        });
        dataConsumer.on('@dataproducerclose', () => {
            this._dataConsumers.delete(dataConsumer.id);
            if (this._sctpStreamIds)
                this._sctpStreamIds[sctpStreamId] = 0;
        });
        // Emit observer event.
        this._observer.safeEmit('newdataconsumer', dataConsumer);
        return dataConsumer;
    }
    /**
     * Enable 'trace' event.
     */
    async enableTraceEvent(types = []) {
        logger.debug('pause()');
        const reqData = { types };
        await this._channel.request('transport.enableTraceEvent', this._internal, reqData);
    }
    _getNextSctpStreamId() {
        if (!this._data.sctpParameters ||
            typeof this._data.sctpParameters.MIS !== 'number') {
            throw new TypeError('missing data.sctpParameters.MIS');
        }
        const numStreams = this._data.sctpParameters.MIS;
        if (!this._sctpStreamIds)
            this._sctpStreamIds = Buffer.alloc(numStreams, 0);
        let sctpStreamId;
        for (let idx = 0; idx < this._sctpStreamIds.length; ++idx) {
            sctpStreamId = (this._nextSctpStreamId + idx) % this._sctpStreamIds.length;
            if (!this._sctpStreamIds[sctpStreamId]) {
                this._nextSctpStreamId = sctpStreamId + 1;
                return sctpStreamId;
            }
        }
        throw new Error('no sctpStreamId available');
    }
}
exports.Transport = Transport;
