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
var _data;
Object.defineProperty(exports, "__esModule", { value: true });
const Logger_1 = require("./Logger");
const Transport_1 = require("./Transport");
const logger = new Logger_1.Logger('PlainTransport');
class PlainTransport extends Transport_1.Transport {
    /**
     * @private
     * @emits tuple - (tuple: TransportTuple)
     * @emits rtcptuple - (rtcpTuple: TransportTuple)
     * @emits sctpstatechange - (sctpState: SctpState)
     * @emits trace - (trace: TransportTraceEventData)
     */
    constructor(params) {
        super(params);
        // PlainTransport data.
        _data.set(this, void 0);
        logger.debug('constructor()');
        const { data } = params;
        __classPrivateFieldSet(this, _data, {
            rtcpMux: data.rtcpMux,
            comedia: data.comedia,
            tuple: data.tuple,
            rtcpTuple: data.rtcpTuple,
            sctpParameters: data.sctpParameters,
            sctpState: data.sctpState,
            srtpParameters: data.srtpParameters
        });
        this.handleWorkerNotifications();
    }
    /**
     * Transport tuple.
     */
    get tuple() {
        return __classPrivateFieldGet(this, _data).tuple;
    }
    /**
     * Transport RTCP tuple.
     */
    get rtcpTuple() {
        return __classPrivateFieldGet(this, _data).rtcpTuple;
    }
    /**
     * SCTP parameters.
     */
    get sctpParameters() {
        return __classPrivateFieldGet(this, _data).sctpParameters;
    }
    /**
     * SCTP state.
     */
    get sctpState() {
        return __classPrivateFieldGet(this, _data).sctpState;
    }
    /**
     * SRTP parameters.
     */
    get srtpParameters() {
        return __classPrivateFieldGet(this, _data).srtpParameters;
    }
    /**
     * Observer.
     *
     * @override
     * @emits close
     * @emits newproducer - (producer: Producer)
     * @emits newconsumer - (consumer: Consumer)
     * @emits newdataproducer - (dataProducer: DataProducer)
     * @emits newdataconsumer - (dataConsumer: DataConsumer)
     * @emits tuple - (tuple: TransportTuple)
     * @emits rtcptuple - (rtcpTuple: TransportTuple)
     * @emits sctpstatechange - (sctpState: SctpState)
     * @emits trace - (trace: TransportTraceEventData)
     */
    // get observer(): EnhancedEventEmitter
    /**
     * Close the PlainTransport.
     *
     * @override
     */
    close() {
        if (this.closed)
            return;
        if (__classPrivateFieldGet(this, _data).sctpState)
            __classPrivateFieldGet(this, _data).sctpState = 'closed';
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
        if (__classPrivateFieldGet(this, _data).sctpState)
            __classPrivateFieldGet(this, _data).sctpState = 'closed';
        super.routerClosed();
    }
    /**
     * Get PlainTransport stats.
     *
     * @override
     */
    async getStats() {
        logger.debug('getStats()');
        return this.channel.request('transport.getStats', this.internal);
    }
    /**
     * Provide the PlainTransport remote parameters.
     *
     * @override
     */
    async connect({ ip, port, rtcpPort, srtpParameters }) {
        logger.debug('connect()');
        const reqData = { ip, port, rtcpPort, srtpParameters };
        const data = await this.channel.request('transport.connect', this.internal, reqData);
        // Update data.
        if (data.tuple)
            __classPrivateFieldGet(this, _data).tuple = data.tuple;
        if (data.rtcpTuple)
            __classPrivateFieldGet(this, _data).rtcpTuple = data.rtcpTuple;
        __classPrivateFieldGet(this, _data).srtpParameters = data.srtpParameters;
    }
    handleWorkerNotifications() {
        this.channel.on(this.internal.transportId, (event, data) => {
            switch (event) {
                case 'tuple':
                    {
                        const tuple = data.tuple;
                        __classPrivateFieldGet(this, _data).tuple = tuple;
                        this.safeEmit('tuple', tuple);
                        // Emit observer event.
                        this.observer.safeEmit('tuple', tuple);
                        break;
                    }
                case 'rtcptuple':
                    {
                        const rtcpTuple = data.rtcpTuple;
                        __classPrivateFieldGet(this, _data).rtcpTuple = rtcpTuple;
                        this.safeEmit('rtcptuple', rtcpTuple);
                        // Emit observer event.
                        this.observer.safeEmit('rtcptuple', rtcpTuple);
                        break;
                    }
                case 'sctpstatechange':
                    {
                        const sctpState = data.sctpState;
                        __classPrivateFieldGet(this, _data).sctpState = sctpState;
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
exports.PlainTransport = PlainTransport;
_data = new WeakMap();
/**
 * DEPRECATED: Use PlainTransport.
 */
class PlainRtpTransport extends PlainTransport {
    constructor(params) {
        super(params);
    }
}
exports.PlainRtpTransport = PlainRtpTransport;
