"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.parseBaseTransportDump = exports.parseSctpListenerDump = exports.parseRtpListenerDump = exports.Transport = void 0;
const uuid_1 = require("uuid");
const Logger_1 = require("./Logger");
const EnhancedEventEmitter_1 = require("./EnhancedEventEmitter");
const utils = require("./utils");
const ortc = require("./ortc");
const WebRtcTransport_1 = require("./WebRtcTransport");
const Producer_1 = require("./Producer");
const Consumer_1 = require("./Consumer");
const DataProducer_1 = require("./DataProducer");
const DataConsumer_1 = require("./DataConsumer");
const RtpParameters_1 = require("./RtpParameters");
const SctpParameters_1 = require("./SctpParameters");
const request_generated_1 = require("./fbs/request_generated");
const response_generated_1 = require("./fbs/response_generated");
const media_kind_1 = require("./fbs/fbs/rtp-parameters/media-kind");
const consumer_generated_1 = require("./fbs/consumer_generated");
const FbsTransport = require("./fbs/transport_generated");
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
     */
    constructor({ internal, data, channel, payloadChannel, appData, getRouterRtpCapabilities, getProducerById, getDataProducerById }) {
        super();
        logger.debug('constructor()');
        this.internal = internal;
        this.#data = data;
        this.channel = channel;
        this.payloadChannel = payloadChannel;
        this.#appData = appData || {};
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
        const reqData = { transportId: this.internal.transportId };
        this.channel.request('router.closeTransport', this.internal.routerId, reqData)
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
     * Listen server was closed (this just happens in WebRtcTransports when their
     * associated WebRtcServer is closed).
     *
     * @private
     */
    listenServerClosed() {
        if (this.#closed)
            return;
        logger.debug('listenServerClosed()');
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
        // Need to emit this event to let the parent Router know since
        // transport.listenServerClosed() is called by the listen server.
        // NOTE: Currently there is just WebRtcServer for WebRtcTransports.
        this.emit('@listenserverclose');
        this.safeEmit('listenserverclose');
        // Emit observer event.
        this.#observer.safeEmit('close');
    }
    /**
     * Dump Transport.
     */
    async dump() {
        logger.debug('dump()');
        const response = await this.channel.requestBinary(request_generated_1.Method.TRANSPORT_DUMP, undefined, undefined, this.internal.transportId);
        /* Decode the response. */
        const dump = new FbsTransport.TransportDumpResponse();
        response.body(dump);
        // TODO: Fix: Consider all transport types.
        const transportDump = new FbsTransport.WebRtcTransportDump();
        dump.data(transportDump);
        return (0, WebRtcTransport_1.parseWebRtcTransportDump)(transportDump);
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
        await this.channel.request('transport.setMaxIncomingBitrate', this.internal.transportId, reqData);
    }
    /**
     * Set maximum outgoing bitrate for sending media.
     */
    async setMaxOutgoingBitrate(bitrate) {
        logger.debug('setMaxOutgoingBitrate() [bitrate:%s]', bitrate);
        const reqData = { bitrate };
        await this.channel.request('transport.setMaxOutgoingBitrate', this.internal.transportId, reqData);
    }
    /**
     * Create a Producer.
     */
    async produce({ id = undefined, kind, rtpParameters, paused = false, keyFrameRequestDelay, appData }) {
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
        const reqData = {
            producerId: id || (0, uuid_1.v4)(),
            kind,
            rtpParameters,
            rtpMapping,
            keyFrameRequestDelay,
            paused
        };
        const status = await this.channel.request('transport.produce', this.internal.transportId, reqData);
        const data = {
            kind,
            rtpParameters,
            type: status.type,
            consumableRtpParameters
        };
        const producer = new Producer_1.Producer({
            internal: {
                ...this.internal,
                producerId: reqData.producerId
            },
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
    async consume({ producerId, rtpCapabilities, paused = false, mid, preferredLayers, ignoreDtx = false, pipe = false, appData }) {
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
        const consumerId = (0, uuid_1.v4)();
        const consumeRequestOffset = this.createConsumeRequest({
            producer,
            consumerId,
            rtpParameters,
            paused,
            preferredLayers,
            ignoreDtx,
            pipe
        });
        const response = await this.channel.requestBinary(request_generated_1.Method.TRANSPORT_CONSUME, request_generated_1.Body.FBS_Transport_ConsumeRequest, consumeRequestOffset, this.internal.transportId);
        /* Decode the response. */
        const consumeResponse = new response_generated_1.ConsumeResponse();
        response.body(consumeResponse);
        const status = consumeResponse.unpack();
        const data = {
            producerId,
            kind: producer.kind,
            rtpParameters,
            type: pipe ? 'pipe' : producer.type
        };
        const consumer = new Consumer_1.Consumer({
            internal: {
                ...this.internal,
                consumerId
            },
            data,
            channel: this.channel,
            payloadChannel: this.payloadChannel,
            appData,
            paused: status.paused,
            producerPaused: status.producerPaused,
            score: status.score ?? undefined,
            preferredLayers: status.preferredLayers ?? undefined
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
    async produceData({ id = undefined, sctpStreamParameters, label = '', protocol = '', appData } = {}) {
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
        const reqData = {
            dataProducerId: id || (0, uuid_1.v4)(),
            type,
            sctpStreamParameters,
            label,
            protocol
        };
        const data = await this.channel.request('transport.produceData', this.internal.transportId, reqData);
        const dataProducer = new DataProducer_1.DataProducer({
            internal: {
                ...this.internal,
                dataProducerId: reqData.dataProducerId
            },
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
    async consumeData({ dataProducerId, ordered, maxPacketLifeTime, maxRetransmits, appData }) {
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
        const reqData = {
            dataConsumerId: (0, uuid_1.v4)(),
            dataProducerId,
            type,
            sctpStreamParameters,
            label,
            protocol
        };
        const data = await this.channel.request('transport.consumeData', this.internal.transportId, reqData);
        const dataConsumer = new DataConsumer_1.DataConsumer({
            internal: {
                ...this.internal,
                dataConsumerId: reqData.dataConsumerId
            },
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
        await this.channel.request('transport.enableTraceEvent', this.internal.transportId, reqData);
    }
    getNextSctpStreamId() {
        if (!this.#data.sctpParameters ||
            typeof this.#data.sctpParameters.MIS !== 'number') {
            throw new TypeError('missing sctpParameters.MIS');
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
    /**
     * flatbuffers helpers
     */
    createConsumeRequest({ producer, consumerId, rtpParameters, paused, preferredLayers, ignoreDtx, pipe }) {
        // Build the request.
        const builder = this.channel.bufferBuilder;
        const rtpParametersOffset = (0, RtpParameters_1.serializeRtpParameters)(builder, rtpParameters);
        const consumerIdOffset = builder.createString(consumerId);
        const producerIdOffset = builder.createString(producer.id);
        let consumableRtpEncodingsOffset;
        let preferredLayersOffset;
        if (producer.consumableRtpParameters.encodings) {
            consumableRtpEncodingsOffset = (0, RtpParameters_1.serializeRtpEncodingParameters)(builder, producer.consumableRtpParameters.encodings);
        }
        if (preferredLayers) {
            // NOTE: Add the maximum possible temporae layer if not provided.
            consumer_generated_1.ConsumerLayers.createConsumerLayers(builder, preferredLayers.spatialLayer, preferredLayers.temporalLayer ?? 255);
        }
        // Create Consume Request.
        request_generated_1.ConsumeRequest.startConsumeRequest(builder);
        request_generated_1.ConsumeRequest.addConsumerId(builder, consumerIdOffset);
        request_generated_1.ConsumeRequest.addProducerId(builder, producerIdOffset);
        request_generated_1.ConsumeRequest.addKind(builder, producer.kind === 'audio' ? media_kind_1.MediaKind.AUDIO : media_kind_1.MediaKind.VIDEO);
        request_generated_1.ConsumeRequest.addRtpParameters(builder, rtpParametersOffset);
        request_generated_1.ConsumeRequest.addType(builder, utils.getRtpParametersType(producer.type, pipe));
        if (consumableRtpEncodingsOffset)
            request_generated_1.ConsumeRequest.addConsumableRtpEncodings(builder, consumableRtpEncodingsOffset);
        request_generated_1.ConsumeRequest.addPaused(builder, paused);
        if (preferredLayersOffset)
            request_generated_1.ConsumeRequest.addPreferredLayers(builder, preferredLayersOffset);
        request_generated_1.ConsumeRequest.addIgnoreDtx(builder, Boolean(ignoreDtx));
        return request_generated_1.ConsumeRequest.endConsumeRequest(builder);
    }
}
exports.Transport = Transport;
function parseRtpListenerDump(binary) {
    // Retrieve ssrcTable.
    const ssrcTable = utils.parseUint32StringVector(binary, 'ssrcTable');
    // Retrieve midTable.
    const midTable = utils.parseUint32StringVector(binary, 'midTable');
    // Retrieve ridTable.
    const ridTable = utils.parseUint32StringVector(binary, 'ridTable');
    return {
        ssrcTable,
        midTable,
        ridTable
    };
}
exports.parseRtpListenerDump = parseRtpListenerDump;
function parseSctpListenerDump(binary) {
    // Retrieve streamIdTable.
    const streamIdTable = utils.parseUint32StringVector(binary, 'streamIdTable');
    return {
        streamIdTable
    };
}
exports.parseSctpListenerDump = parseSctpListenerDump;
function parseBaseTransportDump(binary) {
    // Retrieve producerIds.
    const producerIds = utils.parseVector(binary, 'producerIds');
    // Retrieve consumerIds.
    const consumerIds = utils.parseVector(binary, 'consumerIds');
    // Retrieve map SSRC consumerId.
    const mapSsrcConsumerId = utils.parseUint32StringVector(binary, 'mapSsrcConsumerId');
    // Retrieve map RTX SSRC consumerId.
    const mapRtxSsrcConsumerId = utils.parseUint32StringVector(binary, 'mapRtxSsrcConsumerId');
    // Retrieve dataProducerIds.
    const dataProducerIds = utils.parseVector(binary, 'dataProducerIds');
    // Retrieve dataConsumerIds.
    const dataConsumerIds = utils.parseVector(binary, 'dataConsumerIds');
    // Retrieve recvRtpHeaderExtesions.
    const recvRtpHeaderExtensions = utils.parseStringUint8Vector(binary, 'recvRtpHeaderExtensions');
    // Retrieve RtpListener.
    const rtpListener = parseRtpListenerDump(binary.rtpListener());
    // Retrieve SctpParameters.
    const fbsSctpParameters = binary.sctpParameters();
    let sctpParameters;
    if (fbsSctpParameters) {
        sctpParameters = (0, SctpParameters_1.parseSctpParametersDump)(fbsSctpParameters);
    }
    // Retrieve sctpState.
    const sctpState = binary.sctpState() === '' ? undefined : binary.sctpState();
    // Retrive sctpListener.
    const sctpListener = binary.sctpListener() ?
        parseSctpListenerDump(binary.sctpListener()) :
        undefined;
    // Retrieve traceEventTypes.
    const traceEventTypes = utils.parseVector(binary, 'traceEventTypes');
    return {
        id: binary.id(),
        direct: binary.direct(),
        producerIds: producerIds,
        consumerIds: consumerIds,
        mapSsrcConsumerId: mapSsrcConsumerId,
        mapRtxSsrcConsumerId: mapRtxSsrcConsumerId,
        dataProducerIds: dataProducerIds,
        dataConsumerIds: dataConsumerIds,
        recvRtpHeaderExtensions: recvRtpHeaderExtensions,
        rtpListener: rtpListener,
        maxMessageSize: binary.maxMessageSize(),
        sctpParameters: sctpParameters,
        sctpState: sctpState,
        sctpListener: sctpListener,
        traceEventTypes: traceEventTypes
    };
}
exports.parseBaseTransportDump = parseBaseTransportDump;
