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
const ortc = __importStar(require("./ortc"));
const WebRtcTransport_1 = __importDefault(require("./WebRtcTransport"));
const PlainRtpTransport_1 = __importDefault(require("./PlainRtpTransport"));
const PipeTransport_1 = __importDefault(require("./PipeTransport"));
const AudioLevelObserver_1 = __importDefault(require("./AudioLevelObserver"));
const logger = new Logger_1.default('Router');
class Router extends EnhancedEventEmitter_1.default {
    /**
     * @private
     * @emits workerclose
     * @emits @close
     */
    constructor({ internal, data, channel, appData }) {
        super(logger);
        // Closed flag.
        this._closed = false;
        // Transports map.
        this._transports = new Map();
        // Producers map.
        this._producers = new Map();
        // RtpObservers map.
        this._rtpObservers = new Map();
        // DataProducers map.
        this._dataProducers = new Map();
        // Router to PipeTransport map.
        this._mapRouterPipeTransports = new Map();
        // Observer instance.
        this._observer = new EnhancedEventEmitter_1.default();
        logger.debug('constructor()');
        this._internal = internal;
        this._data = data;
        this._channel = channel;
        this._appData = appData;
    }
    /**
     * Router id.
     */
    get id() {
        return this._internal.routerId;
    }
    /**
     * Whether the Router is closed.
     */
    get closed() {
        return this._closed;
    }
    /**
     * RTC capabilities of the Router.
     */
    get rtpCapabilities() {
        return this._data.rtpCapabilities;
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
     * @emits newtransport - (transport: Transport)
     * @emits newrtpobserver - (rtpObserver: RtpObserver)
     */
    get observer() {
        return this._observer;
    }
    /**
     * Close the Router.
     */
    close() {
        if (this._closed)
            return;
        logger.debug('close()');
        this._closed = true;
        this._channel.request('router.close', this._internal)
            .catch(() => { });
        // Close every Transport.
        for (const transport of this._transports.values()) {
            transport.routerClosed();
        }
        this._transports.clear();
        // Clear the Producers map.
        this._producers.clear();
        // Close every RtpObserver.
        for (const rtpObserver of this._rtpObservers.values()) {
            rtpObserver.routerClosed();
        }
        this._rtpObservers.clear();
        // Clear the DataProducers map.
        this._dataProducers.clear();
        // Clear map of Router/PipeTransports.
        this._mapRouterPipeTransports.clear();
        this.emit('@close');
        // Emit observer event.
        this._observer.safeEmit('close');
    }
    /**
     * Worker was closed.
     *
     * @private
     */
    workerClosed() {
        if (this._closed)
            return;
        logger.debug('workerClosed()');
        this._closed = true;
        // Close every Transport.
        for (const transport of this._transports.values()) {
            transport.routerClosed();
        }
        this._transports.clear();
        // Clear the Producers map.
        this._producers.clear();
        // Close every RtpObserver.
        for (const rtpObserver of this._rtpObservers.values()) {
            rtpObserver.routerClosed();
        }
        this._rtpObservers.clear();
        // Clear the DataProducers map.
        this._dataProducers.clear();
        // Clear map of Router/PipeTransports.
        this._mapRouterPipeTransports.clear();
        this.safeEmit('workerclose');
        // Emit observer event.
        this._observer.safeEmit('close');
    }
    /**
     * Dump Router.
     */
    dump() {
        return __awaiter(this, void 0, void 0, function* () {
            logger.debug('dump()');
            return this._channel.request('router.dump', this._internal);
        });
    }
    /**
     * Create a WebRtcTransport.
     */
    createWebRtcTransport({ listenIps, enableUdp = true, enableTcp = false, preferUdp = false, preferTcp = false, initialAvailableOutgoingBitrate = 600000, enableSctp = false, numSctpStreams = { OS: 1024, MIS: 1024 }, maxSctpMessageSize = 262144, appData = {} }) {
        return __awaiter(this, void 0, void 0, function* () {
            logger.debug('createWebRtcTransport()');
            if (!Array.isArray(listenIps))
                throw new TypeError('missing listenIps');
            else if (appData && typeof appData !== 'object')
                throw new TypeError('if given, appData must be an object');
            listenIps = listenIps.map((listenIp) => {
                if (typeof listenIp === 'string' && listenIp) {
                    return { ip: listenIp };
                }
                else if (typeof listenIp === 'object') {
                    return {
                        ip: listenIp.ip,
                        announcedIp: listenIp.announcedIp || undefined
                    };
                }
                else {
                    throw new TypeError('wrong listenIp');
                }
            });
            const internal = Object.assign(Object.assign({}, this._internal), { transportId: v4_1.default() });
            const reqData = {
                listenIps,
                enableUdp,
                enableTcp,
                preferUdp,
                preferTcp,
                initialAvailableOutgoingBitrate,
                enableSctp,
                numSctpStreams,
                maxSctpMessageSize,
                isDataChannel: true
            };
            const data = yield this._channel.request('router.createWebRtcTransport', internal, reqData);
            const transport = new WebRtcTransport_1.default({
                internal,
                data,
                channel: this._channel,
                appData,
                getRouterRtpCapabilities: () => this._data.rtpCapabilities,
                getProducerById: (producerId) => (this._producers.get(producerId)),
                getDataProducerById: (dataProducerId) => (this._dataProducers.get(dataProducerId))
            });
            this._transports.set(transport.id, transport);
            transport.on('@close', () => this._transports.delete(transport.id));
            transport.on('@newproducer', (producer) => this._producers.set(producer.id, producer));
            transport.on('@producerclose', (producer) => this._producers.delete(producer.id));
            transport.on('@newdataproducer', (dataProducer) => (this._dataProducers.set(dataProducer.id, dataProducer)));
            transport.on('@dataproducerclose', (dataProducer) => (this._dataProducers.delete(dataProducer.id)));
            // Emit observer event.
            this._observer.safeEmit('newtransport', transport);
            return transport;
        });
    }
    /**
     * Create a PlainRtpTransport.
     */
    createPlainRtpTransport({ listenIp, rtcpMux = true, comedia = false, multiSource = false, enableSctp = false, numSctpStreams = { OS: 1024, MIS: 1024 }, maxSctpMessageSize = 262144, appData = {} }) {
        return __awaiter(this, void 0, void 0, function* () {
            logger.debug('createPlainRtpTransport()');
            if (!listenIp)
                throw new TypeError('missing listenIp');
            else if (appData && typeof appData !== 'object')
                throw new TypeError('if given, appData must be an object');
            if (typeof listenIp === 'string' && listenIp) {
                listenIp = { ip: listenIp };
            }
            else if (typeof listenIp === 'object') {
                listenIp =
                    {
                        ip: listenIp.ip,
                        announcedIp: listenIp.announcedIp || undefined
                    };
            }
            else {
                throw new TypeError('wrong listenIp');
            }
            const internal = Object.assign(Object.assign({}, this._internal), { transportId: v4_1.default() });
            const reqData = {
                listenIp,
                rtcpMux,
                comedia,
                multiSource,
                enableSctp,
                numSctpStreams,
                maxSctpMessageSize,
                isDataChannel: false
            };
            const data = yield this._channel.request('router.createPlainRtpTransport', internal, reqData);
            const transport = new PlainRtpTransport_1.default({
                internal,
                data,
                channel: this._channel,
                appData,
                getRouterRtpCapabilities: () => this._data.rtpCapabilities,
                getProducerById: (producerId) => (this._producers.get(producerId)),
                getDataProducerById: (dataProducerId) => (this._dataProducers.get(dataProducerId))
            });
            this._transports.set(transport.id, transport);
            transport.on('@close', () => this._transports.delete(transport.id));
            transport.on('@newproducer', (producer) => this._producers.set(producer.id, producer));
            transport.on('@producerclose', (producer) => this._producers.delete(producer.id));
            transport.on('@newdataproducer', (dataProducer) => (this._dataProducers.set(dataProducer.id, dataProducer)));
            transport.on('@dataproducerclose', (dataProducer) => (this._dataProducers.delete(dataProducer.id)));
            // Emit observer event.
            this._observer.safeEmit('newtransport', transport);
            return transport;
        });
    }
    /**
     * Create a PipeTransport.
     */
    createPipeTransport({ listenIp, enableSctp = false, numSctpStreams = { OS: 1024, MIS: 1024 }, maxSctpMessageSize = 1073741823, appData = {} }) {
        return __awaiter(this, void 0, void 0, function* () {
            logger.debug('createPipeTransport()');
            if (!listenIp)
                throw new TypeError('missing listenIp');
            else if (appData && typeof appData !== 'object')
                throw new TypeError('if given, appData must be an object');
            if (typeof listenIp === 'string' && listenIp) {
                listenIp = { ip: listenIp };
            }
            else if (typeof listenIp === 'object') {
                listenIp =
                    {
                        ip: listenIp.ip,
                        announcedIp: listenIp.announcedIp || undefined
                    };
            }
            else {
                throw new TypeError('wrong listenIp');
            }
            const internal = Object.assign(Object.assign({}, this._internal), { transportId: v4_1.default() });
            const reqData = {
                listenIp,
                enableSctp,
                numSctpStreams,
                maxSctpMessageSize,
                isDataChannel: false
            };
            const data = yield this._channel.request('router.createPipeTransport', internal, reqData);
            const transport = new PipeTransport_1.default({
                internal,
                data,
                channel: this._channel,
                appData,
                getRouterRtpCapabilities: () => this._data.rtpCapabilities,
                getProducerById: (producerId) => (this._producers.get(producerId)),
                getDataProducerById: (dataProducerId) => (this._dataProducers.get(dataProducerId))
            });
            this._transports.set(transport.id, transport);
            transport.on('@close', () => this._transports.delete(transport.id));
            transport.on('@newproducer', (producer) => this._producers.set(producer.id, producer));
            transport.on('@producerclose', (producer) => this._producers.delete(producer.id));
            transport.on('@newdataproducer', (dataProducer) => (this._dataProducers.set(dataProducer.id, dataProducer)));
            transport.on('@dataproducerclose', (dataProducer) => (this._dataProducers.delete(dataProducer.id)));
            // Emit observer event.
            this._observer.safeEmit('newtransport', transport);
            return transport;
        });
    }
    /**
     * Pipes the given Producer or DataProducer into another Router in same host.
     */
    pipeToRouter({ producerId, dataProducerId, router, listenIp = '127.0.0.1', enableSctp = true, numSctpStreams = { OS: 1024, MIS: 1024 } }) {
        return __awaiter(this, void 0, void 0, function* () {
            if (!producerId && !dataProducerId)
                throw new TypeError('missing producerId or dataProducerId');
            else if (producerId && dataProducerId)
                throw new TypeError('just producerId or dataProducerId can be given');
            else if (!router)
                throw new TypeError('Router not found');
            else if (router === this)
                throw new TypeError('cannot use this Router as destination');
            let producer;
            let dataProducer;
            if (producerId) {
                producer = this._producers.get(producerId);
                if (!producer)
                    throw new TypeError('Producer not found');
            }
            else if (dataProducerId) {
                dataProducer = this._dataProducers.get(dataProducerId);
                if (!dataProducer)
                    throw new TypeError('DataProducer not found');
            }
            let pipeTransportPair = this._mapRouterPipeTransports.get(router);
            let localPipeTransport;
            let remotePipeTransport;
            if (pipeTransportPair) {
                localPipeTransport = pipeTransportPair[0];
                remotePipeTransport = pipeTransportPair[1];
            }
            else {
                try {
                    pipeTransportPair = yield Promise.all([
                        this.createPipeTransport({ listenIp, enableSctp, numSctpStreams }),
                        router.createPipeTransport({ listenIp, enableSctp, numSctpStreams })
                    ]);
                    localPipeTransport = pipeTransportPair[0];
                    remotePipeTransport = pipeTransportPair[1];
                    yield Promise.all([
                        localPipeTransport.connect({
                            ip: remotePipeTransport.tuple.localIp,
                            port: remotePipeTransport.tuple.localPort
                        }),
                        remotePipeTransport.connect({
                            ip: localPipeTransport.tuple.localIp,
                            port: localPipeTransport.tuple.localPort
                        })
                    ]);
                    localPipeTransport.observer.on('close', () => {
                        remotePipeTransport.close();
                        this._mapRouterPipeTransports.delete(router);
                    });
                    remotePipeTransport.observer.on('close', () => {
                        localPipeTransport.close();
                        this._mapRouterPipeTransports.delete(router);
                    });
                    this._mapRouterPipeTransports.set(router, [localPipeTransport, remotePipeTransport]);
                }
                catch (error) {
                    logger.error('pipeToRouter() | error creating PipeTransport pair:%o', error);
                    if (localPipeTransport)
                        localPipeTransport.close();
                    if (remotePipeTransport)
                        remotePipeTransport.close();
                    throw error;
                }
            }
            if (producer) {
                let pipeConsumer;
                let pipeProducer;
                try {
                    pipeConsumer = yield localPipeTransport.consume({ producerId });
                    pipeProducer = yield remotePipeTransport.produce({
                        id: producer.id,
                        kind: pipeConsumer.kind,
                        rtpParameters: pipeConsumer.rtpParameters,
                        paused: pipeConsumer.producerPaused,
                        appData: producer.appData
                    });
                    // Pipe events from the pipe Consumer to the pipe Producer.
                    pipeConsumer.observer.on('close', () => pipeProducer.close());
                    pipeConsumer.observer.on('pause', () => pipeProducer.pause());
                    pipeConsumer.observer.on('resume', () => pipeProducer.resume());
                    // Pipe events from the pipe Producer to the pipe Consumer.
                    pipeProducer.observer.on('close', () => pipeConsumer.close());
                    return { pipeConsumer, pipeProducer };
                }
                catch (error) {
                    logger.error('pipeToRouter() | error creating pipe Consumer/Producer pair:%o', error);
                    if (pipeConsumer)
                        pipeConsumer.close();
                    if (pipeProducer)
                        pipeProducer.close();
                    throw error;
                }
            }
            else if (dataProducer) {
                let pipeDataConsumer;
                let pipeDataProducer;
                try {
                    pipeDataConsumer = yield localPipeTransport.consumeData({
                        dataProducerId
                    });
                    pipeDataProducer = yield remotePipeTransport.produceData({
                        id: dataProducer.id,
                        sctpStreamParameters: pipeDataConsumer.sctpStreamParameters,
                        label: pipeDataConsumer.label,
                        protocol: pipeDataConsumer.protocol,
                        appData: dataProducer.appData
                    });
                    // Pipe events from the pipe DataConsumer to the pipe DataProducer.
                    pipeDataConsumer.observer.on('close', () => pipeDataProducer.close());
                    // Pipe events from the pipe DataProducer to the pipe DataConsumer.
                    pipeDataProducer.observer.on('close', () => pipeDataConsumer.close());
                    return { pipeDataConsumer, pipeDataProducer };
                }
                catch (error) {
                    logger.error('pipeToRouter() | error creating pipe DataConsumer/DataProducer pair:%o', error);
                    if (pipeDataConsumer)
                        pipeDataConsumer.close();
                    if (pipeDataProducer)
                        pipeDataProducer.close();
                    throw error;
                }
            }
            else {
                throw new Error('internal error');
            }
        });
    }
    /**
     * Create an AudioLevelObserver.
     */
    createAudioLevelObserver({ maxEntries = 1, threshold = -80, interval = 1000, appData = {} } = {}) {
        return __awaiter(this, void 0, void 0, function* () {
            logger.debug('createAudioLevelObserver()');
            if (appData && typeof appData !== 'object')
                throw new TypeError('if given, appData must be an object');
            const internal = Object.assign(Object.assign({}, this._internal), { rtpObserverId: v4_1.default() });
            const reqData = { maxEntries, threshold, interval };
            yield this._channel.request('router.createAudioLevelObserver', internal, reqData);
            const audioLevelObserver = new AudioLevelObserver_1.default({
                internal,
                channel: this._channel,
                appData,
                getProducerById: (producerId) => (this._producers.get(producerId))
            });
            this._rtpObservers.set(audioLevelObserver.id, audioLevelObserver);
            audioLevelObserver.on('@close', () => {
                this._rtpObservers.delete(audioLevelObserver.id);
            });
            // Emit observer event.
            this._observer.safeEmit('newrtpobserver', audioLevelObserver);
            return audioLevelObserver;
        });
    }
    /**
     * Check whether the given RTP capabilities can consume the given Producer.
     */
    canConsume({ producerId, rtpCapabilities }) {
        const producer = this._producers.get(producerId);
        if (!producer) {
            logger.error('canConsume() | Producer with id "%s" not found', producerId);
            return false;
        }
        try {
            return ortc.canConsume(producer.consumableRtpParameters, rtpCapabilities);
        }
        catch (error) {
            logger.error('canConsume() | unexpected error: %s', String(error));
            return false;
        }
    }
}
exports.default = Router;
