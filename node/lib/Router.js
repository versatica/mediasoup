"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.Router = void 0;
const uuid_1 = require("uuid");
const Logger_1 = require("./Logger");
const EnhancedEventEmitter_1 = require("./EnhancedEventEmitter");
const ortc = require("./ortc");
const errors_1 = require("./errors");
const WebRtcTransport_1 = require("./WebRtcTransport");
const PlainTransport_1 = require("./PlainTransport");
const PipeTransport_1 = require("./PipeTransport");
const DirectTransport_1 = require("./DirectTransport");
const ActiveSpeakerObserver_1 = require("./ActiveSpeakerObserver");
const AudioLevelObserver_1 = require("./AudioLevelObserver");
const logger = new Logger_1.Logger('Router');
class Router extends EnhancedEventEmitter_1.EnhancedEventEmitter {
    // Internal data.
    #internal;
    // Router data.
    #data;
    // Channel instance.
    #channel;
    // PayloadChannel instance.
    #payloadChannel;
    // Closed flag.
    #closed = false;
    // Custom app data.
    #appData;
    // Transports map.
    #transports = new Map();
    // Producers map.
    #producers = new Map();
    // RtpObservers map.
    #rtpObservers = new Map();
    // DataProducers map.
    #dataProducers = new Map();
    // Map of PipeTransport pair Promises indexed by the id of the Router in
    // which pipeToRouter() was called.
    #mapRouterPairPipeTransportPairPromise = new Map();
    // Observer instance.
    #observer = new EnhancedEventEmitter_1.EnhancedEventEmitter();
    /**
     * @private
     * @emits workerclose
     * @emits @close
     */
    constructor({ internal, data, channel, payloadChannel, appData }) {
        super();
        logger.debug('constructor()');
        this.#internal = internal;
        this.#data = data;
        this.#channel = channel;
        this.#payloadChannel = payloadChannel;
        this.#appData = appData;
    }
    /**
     * Router id.
     */
    get id() {
        return this.#internal.routerId;
    }
    /**
     * Whether the Router is closed.
     */
    get closed() {
        return this.#closed;
    }
    /**
     * RTP capabilities of the Router.
     */
    get rtpCapabilities() {
        return this.#data.rtpCapabilities;
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
     * @emits newtransport - (transport: Transport)
     * @emits newrtpobserver - (rtpObserver: RtpObserver)
     */
    get observer() {
        return this.#observer;
    }
    /**
     * Close the Router.
     */
    close() {
        if (this.#closed)
            return;
        logger.debug('close()');
        this.#closed = true;
        this.#channel.request('router.close', this.#internal)
            .catch(() => { });
        // Close every Transport.
        for (const transport of this.#transports.values()) {
            transport.routerClosed();
        }
        this.#transports.clear();
        // Clear the Producers map.
        this.#producers.clear();
        // Close every RtpObserver.
        for (const rtpObserver of this.#rtpObservers.values()) {
            rtpObserver.routerClosed();
        }
        this.#rtpObservers.clear();
        // Clear the DataProducers map.
        this.#dataProducers.clear();
        this.emit('@close');
        // Emit observer event.
        this.#observer.safeEmit('close');
    }
    /**
     * Worker was closed.
     *
     * @private
     */
    workerClosed() {
        if (this.#closed)
            return;
        logger.debug('workerClosed()');
        this.#closed = true;
        // Close every Transport.
        for (const transport of this.#transports.values()) {
            transport.routerClosed();
        }
        this.#transports.clear();
        // Clear the Producers map.
        this.#producers.clear();
        // Close every RtpObserver.
        for (const rtpObserver of this.#rtpObservers.values()) {
            rtpObserver.routerClosed();
        }
        this.#rtpObservers.clear();
        // Clear the DataProducers map.
        this.#dataProducers.clear();
        this.safeEmit('workerclose');
        // Emit observer event.
        this.#observer.safeEmit('close');
    }
    /**
     * Dump Router.
     */
    async dump() {
        logger.debug('dump()');
        return this.#channel.request('router.dump', this.#internal);
    }
    /**
     * Create a WebRtcTransport.
     */
    async createWebRtcTransport({ listenIps, port, enableUdp = true, enableTcp = false, preferUdp = false, preferTcp = false, initialAvailableOutgoingBitrate = 600000, enableSctp = false, numSctpStreams = { OS: 1024, MIS: 1024 }, maxSctpMessageSize = 262144, sctpSendBufferSize = 262144, appData = {} }) {
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
        const internal = { ...this.#internal, transportId: (0, uuid_1.v4)() };
        const reqData = {
            listenIps,
            port,
            enableUdp,
            enableTcp,
            preferUdp,
            preferTcp,
            initialAvailableOutgoingBitrate,
            enableSctp,
            numSctpStreams,
            maxSctpMessageSize,
            sctpSendBufferSize,
            isDataChannel: true
        };
        const data = await this.#channel.request('router.createWebRtcTransport', internal, reqData);
        const transport = new WebRtcTransport_1.WebRtcTransport({
            internal,
            data,
            channel: this.#channel,
            payloadChannel: this.#payloadChannel,
            appData,
            getRouterRtpCapabilities: () => this.#data.rtpCapabilities,
            getProducerById: (producerId) => (this.#producers.get(producerId)),
            getDataProducerById: (dataProducerId) => (this.#dataProducers.get(dataProducerId))
        });
        this.#transports.set(transport.id, transport);
        transport.on('@close', () => this.#transports.delete(transport.id));
        transport.on('@newproducer', (producer) => this.#producers.set(producer.id, producer));
        transport.on('@producerclose', (producer) => this.#producers.delete(producer.id));
        transport.on('@newdataproducer', (dataProducer) => (this.#dataProducers.set(dataProducer.id, dataProducer)));
        transport.on('@dataproducerclose', (dataProducer) => (this.#dataProducers.delete(dataProducer.id)));
        // Emit observer event.
        this.#observer.safeEmit('newtransport', transport);
        return transport;
    }
    /**
     * Create a PlainTransport.
     */
    async createPlainTransport({ listenIp, port, rtcpMux = true, comedia = false, enableSctp = false, numSctpStreams = { OS: 1024, MIS: 1024 }, maxSctpMessageSize = 262144, sctpSendBufferSize = 262144, enableSrtp = false, srtpCryptoSuite = 'AES_CM_128_HMAC_SHA1_80', appData = {} }) {
        logger.debug('createPlainTransport()');
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
        const internal = { ...this.#internal, transportId: (0, uuid_1.v4)() };
        const reqData = {
            listenIp,
            port,
            rtcpMux,
            comedia,
            enableSctp,
            numSctpStreams,
            maxSctpMessageSize,
            sctpSendBufferSize,
            isDataChannel: false,
            enableSrtp,
            srtpCryptoSuite
        };
        const data = await this.#channel.request('router.createPlainTransport', internal, reqData);
        const transport = new PlainTransport_1.PlainTransport({
            internal,
            data,
            channel: this.#channel,
            payloadChannel: this.#payloadChannel,
            appData,
            getRouterRtpCapabilities: () => this.#data.rtpCapabilities,
            getProducerById: (producerId) => (this.#producers.get(producerId)),
            getDataProducerById: (dataProducerId) => (this.#dataProducers.get(dataProducerId))
        });
        this.#transports.set(transport.id, transport);
        transport.on('@close', () => this.#transports.delete(transport.id));
        transport.on('@newproducer', (producer) => this.#producers.set(producer.id, producer));
        transport.on('@producerclose', (producer) => this.#producers.delete(producer.id));
        transport.on('@newdataproducer', (dataProducer) => (this.#dataProducers.set(dataProducer.id, dataProducer)));
        transport.on('@dataproducerclose', (dataProducer) => (this.#dataProducers.delete(dataProducer.id)));
        // Emit observer event.
        this.#observer.safeEmit('newtransport', transport);
        return transport;
    }
    /**
     * DEPRECATED: Use createPlainTransport().
     */
    async createPlainRtpTransport(options) {
        logger.warn('createPlainRtpTransport() is DEPRECATED, use createPlainTransport()');
        return this.createPlainTransport(options);
    }
    /**
     * Create a PipeTransport.
     */
    async createPipeTransport({ listenIp, port, enableSctp = false, numSctpStreams = { OS: 1024, MIS: 1024 }, maxSctpMessageSize = 268435456, sctpSendBufferSize = 268435456, enableRtx = false, enableSrtp = false, appData = {} }) {
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
        const internal = { ...this.#internal, transportId: (0, uuid_1.v4)() };
        const reqData = {
            listenIp,
            port,
            enableSctp,
            numSctpStreams,
            maxSctpMessageSize,
            sctpSendBufferSize,
            isDataChannel: false,
            enableRtx,
            enableSrtp
        };
        const data = await this.#channel.request('router.createPipeTransport', internal, reqData);
        const transport = new PipeTransport_1.PipeTransport({
            internal,
            data,
            channel: this.#channel,
            payloadChannel: this.#payloadChannel,
            appData,
            getRouterRtpCapabilities: () => this.#data.rtpCapabilities,
            getProducerById: (producerId) => (this.#producers.get(producerId)),
            getDataProducerById: (dataProducerId) => (this.#dataProducers.get(dataProducerId))
        });
        this.#transports.set(transport.id, transport);
        transport.on('@close', () => this.#transports.delete(transport.id));
        transport.on('@newproducer', (producer) => this.#producers.set(producer.id, producer));
        transport.on('@producerclose', (producer) => this.#producers.delete(producer.id));
        transport.on('@newdataproducer', (dataProducer) => (this.#dataProducers.set(dataProducer.id, dataProducer)));
        transport.on('@dataproducerclose', (dataProducer) => (this.#dataProducers.delete(dataProducer.id)));
        // Emit observer event.
        this.#observer.safeEmit('newtransport', transport);
        return transport;
    }
    /**
     * Create a DirectTransport.
     */
    async createDirectTransport({ maxMessageSize = 262144, appData = {} } = {
        maxMessageSize: 262144
    }) {
        logger.debug('createDirectTransport()');
        const internal = { ...this.#internal, transportId: (0, uuid_1.v4)() };
        const reqData = { direct: true, maxMessageSize };
        const data = await this.#channel.request('router.createDirectTransport', internal, reqData);
        const transport = new DirectTransport_1.DirectTransport({
            internal,
            data,
            channel: this.#channel,
            payloadChannel: this.#payloadChannel,
            appData,
            getRouterRtpCapabilities: () => this.#data.rtpCapabilities,
            getProducerById: (producerId) => (this.#producers.get(producerId)),
            getDataProducerById: (dataProducerId) => (this.#dataProducers.get(dataProducerId))
        });
        this.#transports.set(transport.id, transport);
        transport.on('@close', () => this.#transports.delete(transport.id));
        transport.on('@newproducer', (producer) => this.#producers.set(producer.id, producer));
        transport.on('@producerclose', (producer) => this.#producers.delete(producer.id));
        transport.on('@newdataproducer', (dataProducer) => (this.#dataProducers.set(dataProducer.id, dataProducer)));
        transport.on('@dataproducerclose', (dataProducer) => (this.#dataProducers.delete(dataProducer.id)));
        // Emit observer event.
        this.#observer.safeEmit('newtransport', transport);
        return transport;
    }
    /**
     * Pipes the given Producer or DataProducer into another Router in same host.
     */
    async pipeToRouter({ producerId, dataProducerId, router, listenIp = '127.0.0.1', enableSctp = true, numSctpStreams = { OS: 1024, MIS: 1024 }, enableRtx = false, enableSrtp = false }) {
        logger.debug('pipeToRouter()');
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
            producer = this.#producers.get(producerId);
            if (!producer)
                throw new TypeError('Producer not found');
        }
        else if (dataProducerId) {
            dataProducer = this.#dataProducers.get(dataProducerId);
            if (!dataProducer)
                throw new TypeError('DataProducer not found');
        }
        const pipeTransportPairKey = router.id;
        let pipeTransportPairPromise = this.#mapRouterPairPipeTransportPairPromise.get(pipeTransportPairKey);
        let pipeTransportPair;
        let localPipeTransport;
        let remotePipeTransport;
        if (pipeTransportPairPromise) {
            pipeTransportPair = await pipeTransportPairPromise;
            localPipeTransport = pipeTransportPair[this.id];
            remotePipeTransport = pipeTransportPair[router.id];
        }
        else {
            pipeTransportPairPromise = new Promise((resolve, reject) => {
                Promise.all([
                    this.createPipeTransport({ listenIp, enableSctp, numSctpStreams, enableRtx, enableSrtp }),
                    router.createPipeTransport({ listenIp, enableSctp, numSctpStreams, enableRtx, enableSrtp })
                ])
                    .then((pipeTransports) => {
                    localPipeTransport = pipeTransports[0];
                    remotePipeTransport = pipeTransports[1];
                })
                    .then(() => {
                    return Promise.all([
                        localPipeTransport.connect({
                            ip: remotePipeTransport.tuple.localIp,
                            port: remotePipeTransport.tuple.localPort,
                            srtpParameters: remotePipeTransport.srtpParameters
                        }),
                        remotePipeTransport.connect({
                            ip: localPipeTransport.tuple.localIp,
                            port: localPipeTransport.tuple.localPort,
                            srtpParameters: localPipeTransport.srtpParameters
                        })
                    ]);
                })
                    .then(() => {
                    localPipeTransport.observer.on('close', () => {
                        remotePipeTransport.close();
                        this.#mapRouterPairPipeTransportPairPromise.delete(pipeTransportPairKey);
                    });
                    remotePipeTransport.observer.on('close', () => {
                        localPipeTransport.close();
                        this.#mapRouterPairPipeTransportPairPromise.delete(pipeTransportPairKey);
                    });
                    resolve({
                        [this.id]: localPipeTransport,
                        [router.id]: remotePipeTransport
                    });
                })
                    .catch((error) => {
                    logger.error('pipeToRouter() | error creating PipeTransport pair:%o', error);
                    if (localPipeTransport)
                        localPipeTransport.close();
                    if (remotePipeTransport)
                        remotePipeTransport.close();
                    reject(error);
                });
            });
            this.#mapRouterPairPipeTransportPairPromise.set(pipeTransportPairKey, pipeTransportPairPromise);
            router.addPipeTransportPair(this.id, pipeTransportPairPromise);
            await pipeTransportPairPromise;
        }
        if (producer) {
            let pipeConsumer;
            let pipeProducer;
            try {
                pipeConsumer = await localPipeTransport.consume({
                    producerId: producerId
                });
                pipeProducer = await remotePipeTransport.produce({
                    id: producer.id,
                    kind: pipeConsumer.kind,
                    rtpParameters: pipeConsumer.rtpParameters,
                    paused: pipeConsumer.producerPaused,
                    appData: producer.appData
                });
                // Ensure that the producer has not been closed in the meanwhile.
                if (producer.closed)
                    throw new errors_1.InvalidStateError('original Producer closed');
                // Ensure that producer.paused has not changed in the meanwhile and, if
                // so, sych the pipeProducer.
                if (pipeProducer.paused !== producer.paused) {
                    if (producer.paused)
                        await pipeProducer.pause();
                    else
                        await pipeProducer.resume();
                }
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
                pipeDataConsumer = await localPipeTransport.consumeData({
                    dataProducerId: dataProducerId
                });
                pipeDataProducer = await remotePipeTransport.produceData({
                    id: dataProducer.id,
                    sctpStreamParameters: pipeDataConsumer.sctpStreamParameters,
                    label: pipeDataConsumer.label,
                    protocol: pipeDataConsumer.protocol,
                    appData: dataProducer.appData
                });
                // Ensure that the dataProducer has not been closed in the meanwhile.
                if (dataProducer.closed)
                    throw new errors_1.InvalidStateError('original DataProducer closed');
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
    }
    /**
     * @private
     */
    addPipeTransportPair(pipeTransportPairKey, pipeTransportPairPromise) {
        if (this.#mapRouterPairPipeTransportPairPromise.has(pipeTransportPairKey)) {
            throw new Error('given pipeTransportPairKey already exists in this Router');
        }
        this.#mapRouterPairPipeTransportPairPromise.set(pipeTransportPairKey, pipeTransportPairPromise);
        pipeTransportPairPromise
            .then((pipeTransportPair) => {
            const localPipeTransport = pipeTransportPair[this.id];
            // NOTE: No need to do any other cleanup here since that is done by the
            // Router calling this method on us.
            localPipeTransport.observer.on('close', () => {
                this.#mapRouterPairPipeTransportPairPromise.delete(pipeTransportPairKey);
            });
        })
            .catch(() => {
            this.#mapRouterPairPipeTransportPairPromise.delete(pipeTransportPairKey);
        });
    }
    /**
     * Create an ActiveSpeakerObserver
     */
    async createActiveSpeakerObserver({ interval = 300, appData = {} } = {}) {
        logger.debug('createActiveSpeakerObserver()');
        if (appData && typeof appData !== 'object')
            throw new TypeError('if given, appData must be an object');
        const internal = { ...this.#internal, rtpObserverId: (0, uuid_1.v4)() };
        const reqData = { interval };
        await this.#channel.request('router.createActiveSpeakerObserver', internal, reqData);
        const activeSpeakerObserver = new ActiveSpeakerObserver_1.ActiveSpeakerObserver({
            internal,
            channel: this.#channel,
            payloadChannel: this.#payloadChannel,
            appData,
            getProducerById: (producerId) => (this.#producers.get(producerId))
        });
        this.#rtpObservers.set(activeSpeakerObserver.id, activeSpeakerObserver);
        activeSpeakerObserver.on('@close', () => {
            this.#rtpObservers.delete(activeSpeakerObserver.id);
        });
        // Emit observer event.
        this.#observer.safeEmit('newrtpobserver', activeSpeakerObserver);
        return activeSpeakerObserver;
    }
    /**
     * Create an AudioLevelObserver.
     */
    async createAudioLevelObserver({ maxEntries = 1, threshold = -80, interval = 1000, appData = {} } = {}) {
        logger.debug('createAudioLevelObserver()');
        if (appData && typeof appData !== 'object')
            throw new TypeError('if given, appData must be an object');
        const internal = { ...this.#internal, rtpObserverId: (0, uuid_1.v4)() };
        const reqData = { maxEntries, threshold, interval };
        await this.#channel.request('router.createAudioLevelObserver', internal, reqData);
        const audioLevelObserver = new AudioLevelObserver_1.AudioLevelObserver({
            internal,
            channel: this.#channel,
            payloadChannel: this.#payloadChannel,
            appData,
            getProducerById: (producerId) => (this.#producers.get(producerId))
        });
        this.#rtpObservers.set(audioLevelObserver.id, audioLevelObserver);
        audioLevelObserver.on('@close', () => {
            this.#rtpObservers.delete(audioLevelObserver.id);
        });
        // Emit observer event.
        this.#observer.safeEmit('newrtpobserver', audioLevelObserver);
        return audioLevelObserver;
    }
    /**
     * Check whether the given RTP capabilities can consume the given Producer.
     */
    canConsume({ producerId, rtpCapabilities }) {
        const producer = this.#producers.get(producerId);
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
exports.Router = Router;
