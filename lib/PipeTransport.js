"use strict";
var __importStar = (this && this.__importStar) || function (mod) {
    if (mod && mod.__esModule) return mod;
    var result = {};
    if (mod != null) for (var k in mod) if (Object.hasOwnProperty.call(mod, k)) result[k] = mod[k];
    result["default"] = mod;
    return result;
};
Object.defineProperty(exports, "__esModule", { value: true });
const uuid_1 = require("uuid");
const Logger_1 = require("./Logger");
const ortc = __importStar(require("./ortc"));
const Transport_1 = require("./Transport");
const Consumer_1 = require("./Consumer");
const logger = new Logger_1.Logger('PipeTransport');
class PipeTransport extends Transport_1.Transport {
    // PipeTransport data.
    // - .tuple
    //   - .localIp
    //   - .localPort
    //   - .remoteIp
    //   - .remotePort
    //   - .protocol
    // - .sctpParameters
    //   - .port
    //   - .OS
    //   - .MIS
    //   - .maxMessageSize
    // - .sctpState
    // - .rtx
    // - .srtpKey
    /**
     * @private
     * @emits sctpstatechange - (sctpState: SctpState)
     * @emits trace - (trace: TransportTraceEventData)
     */
    constructor(params) {
        super(params);
        logger.debug('constructor()');
        const { data } = params;
        this._data =
            {
                tuple: data.tuple,
                sctpParameters: data.sctpParameters,
                sctpState: data.sctpState,
                rtx: data.rtx,
                srtpKey: data.srtpKey
            };
        this._handleWorkerNotifications();
    }
    /**
     * Transport tuple.
     */
    get tuple() {
        return this._data.tuple;
    }
    /**
     * SCTP parameters.
     */
    get sctpParameters() {
        return this._data.sctpParameters;
    }
    /**
     * SCTP state.
     */
    get sctpState() {
        return this._data.sctpState;
    }
    /**
     * SRTP key.
     */
    get srtpKey() {
        return this._data.srtpKey;
    }
    /**
     * Observer.
     *
     * @override
     * @emits close
     * @emits newproducer - (producer: Producer)
     * @emits newconsumer - (producer: Producer)
     * @emits newdataproducer - (dataProducer: DataProducer)
     * @emits newdataconsumer - (dataProducer: DataProducer)
     * @emits sctpstatechange - (sctpState: SctpState)
     * @emits trace - (trace: TransportTraceEventData)
     */
    get observer() {
        return this._observer;
    }
    /**
     * Close the PlainRtpTransport.
     *
     * @override
     */
    close() {
        if (this._closed)
            return;
        if (this._data.sctpState)
            this._data.sctpState = 'closed';
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
        if (this._data.sctpState)
            this._data.sctpState = 'closed';
        super.routerClosed();
    }
    /**
     * Get PipeTransport stats.
     *
     * @override
     */
    async getStats() {
        logger.debug('getStats()');
        return this._channel.request('transport.getStats', this._internal);
    }
    /**
     * Provide the PipeTransport remote parameters.
     *
     * @override
     */
    async connect({ ip, port, srtpKey }) {
        logger.debug('connect()');
        const reqData = { ip, port, srtpKey };
        const data = await this._channel.request('transport.connect', this._internal, reqData);
        // Update data.
        this._data.tuple = data.tuple;
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
        const producer = this._getProducerById(producerId);
        if (!producer)
            throw Error(`Producer with id "${producerId}" not found`);
        // This may throw.
        const rtpParameters = ortc.getPipeConsumerRtpParameters(producer.consumableRtpParameters, this._data.rtx);
        const internal = { ...this._internal, consumerId: uuid_1.v4(), producerId };
        const reqData = {
            kind: producer.kind,
            rtpParameters,
            type: 'pipe',
            consumableRtpEncodings: producer.consumableRtpParameters.encodings
        };
        const status = await this._channel.request('transport.consume', internal, reqData);
        const data = { kind: producer.kind, rtpParameters, type: 'pipe' };
        const consumer = new Consumer_1.Consumer({
            internal,
            data,
            channel: this._channel,
            appData,
            paused: status.paused,
            producerPaused: status.producerPaused
        });
        this._consumers.set(consumer.id, consumer);
        consumer.on('@close', () => this._consumers.delete(consumer.id));
        consumer.on('@producerclose', () => this._consumers.delete(consumer.id));
        // Emit observer event.
        this._observer.safeEmit('newconsumer', consumer);
        return consumer;
    }
    _handleWorkerNotifications() {
        this._channel.on(this._internal.transportId, (event, data) => {
            switch (event) {
                case 'sctpstatechange':
                    {
                        const sctpState = data.sctpState;
                        this._data.sctpState = sctpState;
                        this.safeEmit('sctpstatechange', sctpState);
                        // Emit observer event.
                        this._observer.safeEmit('sctpstatechange', sctpState);
                        break;
                    }
                case 'trace':
                    {
                        const trace = data;
                        this.safeEmit('trace', trace);
                        // Emit observer event.
                        this._observer.safeEmit('trace', trace);
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
