"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.Transport = void 0;
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
    // Internal data.
    internal;
    // Transport data. This is set by the subclass.
    #data;
    // Channel instance.
    channel;
    // PayloadChannel instance.
    payloadChannel;
    // Close flag.
    #closed = false;
    // Custom app data.
    #appData;
    // Method to retrieve Router RTP capabilities.
    #getRouterRtpCapabilities;
    // Method to retrieve a Producer.
    getProducerById;
    // Method to retrieve a DataProducer.
    getDataProducerById;
    // Producers map.
    #producers = new Map();
    // Consumers map.
    consumers = new Map();
    // DataProducers map.
    dataProducers = new Map();
    // DataConsumers map.
    dataConsumers = new Map();
    // RTCP CNAME for Producers.
    #cnameForProducers;
    // Next MID for Consumers. It's converted into string when used.
    #nextMidForConsumers = 0;
    // Buffer with available SCTP stream ids.
    #sctpStreamIds;
    // Next SCTP stream id.
    #nextSctpStreamId = 0;
    // Observer instance.
    #observer = new EnhancedEventEmitter_1.EnhancedEventEmitter();
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
        logger.debug('constructor()');
        this.internal = internal;
        this.#data = data;
        this.channel = channel;
        this.payloadChannel = payloadChannel;
        this.#appData = appData;
        this.#getRouterRtpCapabilities = getRouterRtpCapabilities;
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
        return this.#closed;
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
     * @emits newproducer - (producer: Producer)
     * @emits newconsumer - (producer: Producer)
     * @emits newdataproducer - (dataProducer: DataProducer)
     * @emits newdataconsumer - (dataProducer: DataProducer)
     */
    get observer() {
        return this.#observer;
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
        if (this.#closed)
            return;
        logger.debug('close()');
        this.#closed = true;
        // Remove notification subscriptions.
        this.channel.removeAllListeners(this.internal.transportId);
        this.payloadChannel.removeAllListeners(this.internal.transportId);
        this.channel.request('transport.close', this.internal)
            .catch(() => { });
        // Close every Producer.
        for (const producer of this.#producers.values()) {
            producer.transportClosed();
            // Must tell the Router.
            this.emit('@producerclose', producer);
        }
        this.#producers.clear();
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
        this.#observer.safeEmit('close');
    }
    /**
     * Router was closed.
     *
     * @private
     * @virtual
     */
    routerClosed() {
        if (this.#closed)
            return;
        logger.debug('routerClosed()');
        this.#closed = true;
        // Remove notification subscriptions.
        this.channel.removeAllListeners(this.internal.transportId);
        this.payloadChannel.removeAllListeners(this.internal.transportId);
        // Close every Producer.
        for (const producer of this.#producers.values()) {
            producer.transportClosed();
            // NOTE: No need to tell the Router since it already knows (it has
            // been closed in fact).
        }
        this.#producers.clear();
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
        this.#observer.safeEmit('close');
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
        if (id && this.#producers.has(id))
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
            if (!this.#cnameForProducers && rtpParameters.rtcp && rtpParameters.rtcp.cname) {
                this.#cnameForProducers = rtpParameters.rtcp.cname;
            }
            // Otherwise if we don't have yet a CNAME for Producers and the RTP parameters
            // do not include CNAME, create a random one.
            else if (!this.#cnameForProducers) {
                this.#cnameForProducers = (0, uuid_1.v4)().substr(0, 8);
            }
            // Override Producer's CNAME.
            rtpParameters.rtcp = rtpParameters.rtcp || {};
            rtpParameters.rtcp.cname = this.#cnameForProducers;
        }
        const routerRtpCapabilities = this.#getRouterRtpCapabilities();
        // This may throw.
        const rtpMapping = ortc.getProducerRtpParametersMapping(rtpParameters, routerRtpCapabilities);
        // This may throw.
        const consumableRtpParameters = ortc.getConsumableRtpParameters(kind, rtpParameters, routerRtpCapabilities, rtpMapping);
        const internal = { ...this.internal, producerId: id || (0, uuid_1.v4)() };
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
        this.#producers.set(producer.id, producer);
        producer.on('@close', () => {
            this.#producers.delete(producer.id);
            this.emit('@producerclose', producer);
        });
        this.emit('@newproducer', producer);
        // Emit observer event.
        this.#observer.safeEmit('newproducer', producer);
        return producer;
    }
    /**
     * Create a Consumer.
     *
     * @virtual
     */
    async consume({ producerId, rtpCapabilities, paused = false, mid, preferredLayers, pipe = false, appData = {} }) {
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
                rtpParameters.mid = `${this.#nextMidForConsumers++}`;
                // We use up to 8 bytes for MID (string).
                if (this.#nextMidForConsumers === 100000000) {
                    logger.error(`consume() | reaching max MID value "${this.#nextMidForConsumers}"`);
                    this.#nextMidForConsumers = 0;
                }
            }
        }
        const internal = { ...this.internal, consumerId: (0, uuid_1.v4)(), producerId };
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
        this.#observer.safeEmit('newconsumer', consumer);
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
        const internal = { ...this.internal, dataProducerId: id || (0, uuid_1.v4)() };
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
        this.#observer.safeEmit('newdataproducer', dataProducer);
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
            this.#sctpStreamIds[sctpStreamId] = 1;
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
        const internal = { ...this.internal, dataConsumerId: (0, uuid_1.v4)(), dataProducerId };
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
            if (this.#sctpStreamIds)
                this.#sctpStreamIds[sctpStreamId] = 0;
        });
        dataConsumer.on('@dataproducerclose', () => {
            this.dataConsumers.delete(dataConsumer.id);
            if (this.#sctpStreamIds)
                this.#sctpStreamIds[sctpStreamId] = 0;
        });
        // Emit observer event.
        this.#observer.safeEmit('newdataconsumer', dataConsumer);
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
        if (!this.#data.sctpParameters ||
            typeof this.#data.sctpParameters.MIS !== 'number') {
            throw new TypeError('missing data.sctpParameters.MIS');
        }
        const numStreams = this.#data.sctpParameters.MIS;
        if (!this.#sctpStreamIds)
            this.#sctpStreamIds = Buffer.alloc(numStreams, 0);
        let sctpStreamId;
        for (let idx = 0; idx < this.#sctpStreamIds.length; ++idx) {
            sctpStreamId = (this.#nextSctpStreamId + idx) % this.#sctpStreamIds.length;
            if (!this.#sctpStreamIds[sctpStreamId]) {
                this.#nextSctpStreamId = sctpStreamId + 1;
                return sctpStreamId;
            }
        }
        throw new Error('no sctpStreamId available');
    }
}
exports.Transport = Transport;
