"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.PipeTransport = void 0;
const uuid_1 = require("uuid");
const Logger_1 = require("./Logger");
const ortc = require("./ortc");
const Transport_1 = require("./Transport");
const Consumer_1 = require("./Consumer");
const logger = new Logger_1.Logger('PipeTransport');
class PipeTransport extends Transport_1.Transport {
    // PipeTransport data.
    #data;
    /**
     * @private
     */
    constructor(options) {
        super(options);
        logger.debug('constructor()');
        const { data } = options;
        this.#data =
            {
                tuple: data.tuple,
                sctpParameters: data.sctpParameters,
                sctpState: data.sctpState,
                rtx: data.rtx,
                srtpParameters: data.srtpParameters
            };
        this.handleWorkerNotifications();
    }
    /**
     * Transport tuple.
     */
    get tuple() {
        return this.#data.tuple;
    }
    /**
     * SCTP parameters.
     */
    get sctpParameters() {
        return this.#data.sctpParameters;
    }
    /**
     * SCTP state.
     */
    get sctpState() {
        return this.#data.sctpState;
    }
    /**
     * SRTP parameters.
     */
    get srtpParameters() {
        return this.#data.srtpParameters;
    }
    /**
     * Close the PipeTransport.
     *
     * @override
     */
    close() {
        if (this.closed)
            return;
        if (this.#data.sctpState)
            this.#data.sctpState = 'closed';
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
        if (this.#data.sctpState)
            this.#data.sctpState = 'closed';
        super.routerClosed();
    }
    /**
     * Get PipeTransport stats.
     *
     * @override
     */
    async getStats() {
        logger.debug('getStats()');
        return this.channel.request('transport.getStats', this.internal.transportId);
    }
    /**
     * Provide the PipeTransport remote parameters.
     *
     * @override
     */
    async connect({ ip, port, srtpParameters }) {
        logger.debug('connect()');
        const reqData = { ip, port, srtpParameters };
        const data = await this.channel.request('transport.connect', this.internal.transportId, reqData);
        // Update data.
        this.#data.tuple = data.tuple;
    }
    /**
     * Create a pipe Consumer.
     *
     * @override
     */
    async consume({ producerId, appData }) {
        logger.debug('consume()');
        if (!producerId || typeof producerId !== 'string')
            throw new TypeError('missing producerId');
        else if (appData && typeof appData !== 'object')
            throw new TypeError('if given, appData must be an object');
        const producer = this.getProducerById(producerId);
        if (!producer)
            throw Error(`Producer with id "${producerId}" not found`);
        // This may throw.
        const rtpParameters = ortc.getPipeConsumerRtpParameters(producer.consumableRtpParameters, this.#data.rtx);
        const reqData = {
            consumerId: (0, uuid_1.v4)(),
            producerId,
            kind: producer.kind,
            rtpParameters,
            type: 'pipe',
            consumableRtpEncodings: producer.consumableRtpParameters.encodings
        };
        const status = await this.channel.request('transport.consume', this.internal.transportId, reqData);
        const data = {
            producerId,
            kind: producer.kind,
            rtpParameters,
            type: 'pipe'
        };
        const consumer = new Consumer_1.Consumer({
            internal: {
                ...this.internal,
                consumerId: reqData.consumerId
            },
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
                        this.#data.sctpState = sctpState;
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
