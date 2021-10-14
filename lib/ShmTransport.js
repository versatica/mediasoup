"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Logger_1 = require("./Logger");
const uuid_1 = require("uuid");
const ortc = require("./ortc");
const Consumer_1 = require("./Consumer");
const Transport_1 = require("./Transport");
const logger = new Logger_1.Logger('ShmTransport');
class ShmTransport extends Transport_1.Transport {
    /**
    * @private
     *
     */
    constructor(params) {
        super(params);
        logger.debug('constructor()');
        const { data } = params;
        // ShmTransport data.
        this._shm = data.shm.name;
        this._log = data.shm.log;
    }
    /**
     * Observer.
     *
     * @override
     * @type {EventEmitter}
     *
     * @emits close
     * @emits {producer: Producer} newproducer
     * @emits {consumer: Consumer} newconsumer
     * @emits {producer: DataProducer} newdataproducer
     * @emits {consumer: DataConsumer} newdataconsumer
     * @emits {sctpState: String} sctpstatechange
     */
    get observer() {
        return this._observer;
    }
    /**
     * Close the ShmTransport.
     *
     * @override
     */
    close() {
        if (this._closed)
            return;
        super.close();
    }
    /**
     * Router was closed.
     *
     * @private
     * @override
     */
    routerClosed() {
        if (this._closed)
            return;
        super.routerClosed();
    }
    /**
     * Get ShmTransport stats.
     *
     * @override
     */
    async getStats() {
        logger.debug('ShmTransport.getStats()');
        return this._channel.request('transport.getStats', this._internal);
    }
    /**
     * Provide the ShmTransport remote parameters.
     *
     * @param {String} shm- shm name.
     *
     * @async
     * @override
     */
    async connect({ shm }) {
        logger.debug('ShmTransport.connect()');
        const reqData = { shm };
        await this._channel.request('transport.connect', this._internal, reqData);
    }
    /**
     * Create a shm Consumer.
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
        // Skipped MID, see in Transport.ts: rtpParameters.mid = `${this._nextMidForConsumers++}`;
        const internal = { ...this._internal, consumerId: uuid_1.v4(), producerId };
        const shmData = appData ?
            {
                shm: (appData.shm !== undefined) ? appData.shm : {},
                log: (appData.log !== undefined) ? appData.log : {}
            }
            : {};
        const reqData = {
            kind: producer.kind,
            rtpParameters,
            type: 'shm',
            shm: shmData,
            consumableRtpEncodings: producer.consumableRtpParameters.encodings,
            paused,
            preferredLayers,
            appData,
        };
        const status = await this._channel.request('transport.consume', internal, reqData);
        const data = { kind: producer.kind, rtpParameters, type: 'shm' };
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
     * Provide the ShmTransport remote parameters.
     *
     * @param {Object} meta - metadata string.
     *
     * @async
     */
    async writeStreamMetaData({ meta }) {
        logger.debug('writeStreamMetaData()');
        const reqData = {
            meta,
            shm: this._shm,
            log: this._log
        };
        await this._channel.request('transport.consumeStreamMeta', this._internal, reqData);
    }
    /**
     * Does nothing, should not be called like this
     *
     * @private
     * @override
     */
    _handleWorkerNotifications() {
        this._channel.on(this._internal.transportId, async (event, data) => {
            switch (event) {
                default:
                    {
                        logger.error('ignoring unknown event "%s"', event);
                    }
            }
        });
    }
}
exports.ShmTransport = ShmTransport;
