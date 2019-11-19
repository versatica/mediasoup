"use strict";
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
var __importStar = (this && this.__importStar) || function (mod) {
    if (mod && mod.__esModule) return mod;
    var result = {};
    if (mod != null) for (var k in mod) if (Object.hasOwnProperty.call(mod, k)) result[k] = mod[k];
    result["default"] = mod;
    return result;
};
Object.defineProperty(exports, "__esModule", { value: true });
const v4_1 = __importDefault(require("uuid/v4"));
const Logger_1 = __importDefault(require("./Logger"));
const EnhancedEventEmitter_1 = __importDefault(require("./EnhancedEventEmitter"));
const utils = __importStar(require("./utils"));
const ortc = __importStar(require("./ortc"));
const Producer_1 = __importDefault(require("./Producer"));
const Consumer_1 = __importDefault(require("./Consumer"));
const DataProducer_1 = __importDefault(require("./DataProducer"));
const DataConsumer_1 = __importDefault(require("./DataConsumer"));
const logger = new Logger_1.default('Transport');
class Transport extends EnhancedEventEmitter_1.default {
    /**
     * @private
     * @interface
     * @emits routerclose
     * @emits @close
     * @emits @newproducer
     * @emits @producerclose
     * @emits @newdataproducer
     * @emits @dataproducerclose
     */
    constructor({ internal, data, channel, appData, getRouterRtpCapabilities, getProducerById, getDataProducerById }) {
        super(logger);
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
        // Next SCTP stream id.
        this._nextSctpStreamId = 0;
        // Observer instance.
        this._observer = new EnhancedEventEmitter_1.default();
        logger.debug('constructor()');
        this._internal = internal;
        this._data = data;
        this._channel = channel;
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
     * @emits {producer: Producer} newproducer
     * @emits {consumer: Consumer} newconsumer
     * @emits {producer: DataProducer} newdataproducer
     * @emits {consumer: DataConsumer} newdataconsumer
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
    dump() {
        return __awaiter(this, void 0, void 0, function* () {
            logger.debug('dump()');
            return this._channel.request('transport.dump', this._internal);
        });
    }
    /**
     * Get Transport stats.
     *
     * @abstract
     */
    getStats() {
        return __awaiter(this, void 0, void 0, function* () {
            // Should not happen.
            throw new Error('method not implemented in the subclass');
        });
    }
    /**
     * Provide the Transport remote parameters.
     *
     * @abstract
     */
    // eslint-disable-next-line @typescript-eslint/no-unused-vars
    connect(params) {
        return __awaiter(this, void 0, void 0, function* () {
            // Should not happen.
            throw new Error('method not implemented in the subclass');
        });
    }
    /**
     * Set maximum incoming bitrate for receiving media.
     */
    setMaxIncomingBitrate(bitrate) {
        return __awaiter(this, void 0, void 0, function* () {
            logger.debug('setMaxIncomingBitrate() [bitrate:%s]', bitrate);
            const reqData = { bitrate };
            yield this._channel.request('transport.setMaxIncomingBitrate', this._internal, reqData);
        });
    }
    /**
     * Create a Producer.
     */
    produce({ id = undefined, kind, rtpParameters, paused = false, appData = {} }) {
        return __awaiter(this, void 0, void 0, function* () {
            logger.debug('produce()');
            if (id && this._producers.has(id))
                throw new TypeError(`a Producer with same id "${id}" already exists`);
            else if (!['audio', 'video'].includes(kind))
                throw new TypeError(`invalid kind "${kind}"`);
            else if (typeof rtpParameters !== 'object')
                throw new TypeError('missing rtpParameters');
            else if (appData && typeof appData !== 'object')
                throw new TypeError('if given, appData must be an object');
            // If missing encodings, add one.
            if (!rtpParameters.encodings || !Array.isArray(rtpParameters.encodings))
                rtpParameters.encodings = [{}];
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
                    this._cnameForProducers = v4_1.default().substr(0, 8);
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
            const internal = Object.assign(Object.assign({}, this._internal), { producerId: id || v4_1.default() });
            const reqData = { kind, rtpParameters, rtpMapping, paused };
            const status = yield this._channel.request('transport.produce', internal, reqData);
            const data = {
                kind,
                rtpParameters,
                type: status.type,
                consumableRtpParameters
            };
            const producer = new Producer_1.default({
                internal,
                data,
                channel: this._channel,
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
        });
    }
    /**
     * Create a Consumer.
     *
     * @virtual
     */
    consume({ producerId, rtpCapabilities, paused = false, preferredLayers, appData = {} }) {
        return __awaiter(this, void 0, void 0, function* () {
            logger.debug('consume()');
            if (!producerId || typeof producerId !== 'string')
                throw new TypeError('missing producerId');
            else if (typeof rtpCapabilities !== 'object')
                throw new TypeError('missing rtpCapabilities');
            else if (appData && typeof appData !== 'object')
                throw new TypeError('if given, appData must be an object');
            const producer = this._getProducerById(producerId);
            if (!producer)
                throw Error(`Producer with id "${producerId}" not found`);
            // This may throw.
            const rtpParameters = ortc.getConsumerRtpParameters(producer.consumableRtpParameters, rtpCapabilities);
            const internal = Object.assign(Object.assign({}, this._internal), { consumerId: v4_1.default(), producerId });
            const reqData = {
                kind: producer.kind,
                rtpParameters,
                type: producer.type,
                consumableRtpEncodings: producer.consumableRtpParameters.encodings,
                paused,
                preferredLayers
            };
            const status = yield this._channel.request('transport.consume', internal, reqData);
            const data = { kind: producer.kind, rtpParameters, type: producer.type };
            const consumer = new Consumer_1.default({
                internal,
                data,
                channel: this._channel,
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
        });
    }
    /**
     * Create a DataProducer.
     */
    produceData({ id = undefined, sctpStreamParameters, label = '', protocol = '', appData = {} }) {
        return __awaiter(this, void 0, void 0, function* () {
            logger.debug('produceData()');
            if (id && this._dataProducers.has(id))
                throw new TypeError(`a DataProducer with same id "${id}" already exists`);
            else if (typeof sctpStreamParameters !== 'object')
                throw new TypeError('missing sctpStreamParameters');
            else if (appData && typeof appData !== 'object')
                throw new TypeError('if given, appData must be an object');
            const internal = Object.assign(Object.assign({}, this._internal), { dataProducerId: id || v4_1.default() });
            const reqData = { sctpStreamParameters, label, protocol };
            const data = yield this._channel.request('transport.produceData', internal, reqData);
            const dataProducer = new DataProducer_1.default({
                internal,
                data,
                channel: this._channel,
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
        });
    }
    /**
     * Create a DataConsumer.
     */
    consumeData({ dataProducerId, appData = {} }) {
        return __awaiter(this, void 0, void 0, function* () {
            logger.debug('consumeData()');
            if (!dataProducerId || typeof dataProducerId !== 'string')
                throw new TypeError('missing dataProducerId');
            else if (appData && typeof appData !== 'object')
                throw new TypeError('if given, appData must be an object');
            const dataProducer = this._getDataProducerById(dataProducerId);
            if (!dataProducer)
                throw Error(`DataProducer with id "${dataProducerId}" not found`);
            const sctpStreamParameters = utils.clone(dataProducer.sctpStreamParameters);
            const { label, protocol } = dataProducer;
            // This may throw.
            const sctpStreamId = this._getNextSctpStreamId();
            this._sctpStreamIds[sctpStreamId] = 1;
            sctpStreamParameters.streamId = sctpStreamId;
            const internal = Object.assign(Object.assign({}, this._internal), { dataConsumerId: v4_1.default(), dataProducerId });
            const reqData = { sctpStreamParameters, label, protocol };
            const data = yield this._channel.request('transport.consumeData', internal, reqData);
            const dataConsumer = new DataConsumer_1.default({
                internal,
                data,
                channel: this._channel,
                appData
            });
            this._dataConsumers.set(dataConsumer.id, dataConsumer);
            dataConsumer.on('@close', () => {
                this._dataConsumers.delete(dataConsumer.id);
                this._sctpStreamIds[sctpStreamId] = 0;
            });
            dataConsumer.on('@dataproducerclose', () => {
                this._dataConsumers.delete(dataConsumer.id);
                this._sctpStreamIds[sctpStreamId] = 0;
            });
            // Emit observer event.
            this._observer.safeEmit('newdataconsumer', dataConsumer);
            return dataConsumer;
        });
    }
    /**
     * Enable 'packet' event.
     */
    enablePacketEvent(types = []) {
        return __awaiter(this, void 0, void 0, function* () {
            logger.debug('pause()');
            const reqData = { types };
            yield this._channel.request('transport.enablePacketEvent', this._internal, reqData);
        });
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
exports.default = Transport;
