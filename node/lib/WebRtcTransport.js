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
const logger = new Logger_1.Logger('WebRtcTransport');
class WebRtcTransport extends Transport_1.Transport {
    /**
     * @private
     * @emits icestatechange - (iceState: IceState)
     * @emits iceselectedtuplechange - (iceSelectedTuple: TransportTuple)
     * @emits dtlsstatechange - (dtlsState: DtlsState)
     * @emits sctpstatechange - (sctpState: SctpState)
     * @emits trace - (trace: TransportTraceEventData)
     */
    constructor(params) {
        super(params);
        // WebRtcTransport data.
        _data.set(this, void 0);
        logger.debug('constructor()');
        const { data } = params;
        __classPrivateFieldSet(this, _data, {
            iceRole: data.iceRole,
            iceParameters: data.iceParameters,
            iceCandidates: data.iceCandidates,
            iceState: data.iceState,
            iceSelectedTuple: data.iceSelectedTuple,
            dtlsParameters: data.dtlsParameters,
            dtlsState: data.dtlsState,
            dtlsRemoteCert: data.dtlsRemoteCert,
            sctpParameters: data.sctpParameters,
            sctpState: data.sctpState
        });
        this.handleWorkerNotifications();
    }
    /**
     * ICE role.
     */
    get iceRole() {
        return __classPrivateFieldGet(this, _data).iceRole;
    }
    /**
     * ICE parameters.
     */
    get iceParameters() {
        return __classPrivateFieldGet(this, _data).iceParameters;
    }
    /**
     * ICE candidates.
     */
    get iceCandidates() {
        return __classPrivateFieldGet(this, _data).iceCandidates;
    }
    /**
     * ICE state.
     */
    get iceState() {
        return __classPrivateFieldGet(this, _data).iceState;
    }
    /**
     * ICE selected tuple.
     */
    get iceSelectedTuple() {
        return __classPrivateFieldGet(this, _data).iceSelectedTuple;
    }
    /**
     * DTLS parameters.
     */
    get dtlsParameters() {
        return __classPrivateFieldGet(this, _data).dtlsParameters;
    }
    /**
     * DTLS state.
     */
    get dtlsState() {
        return __classPrivateFieldGet(this, _data).dtlsState;
    }
    /**
     * Remote certificate in PEM format.
     */
    get dtlsRemoteCert() {
        return __classPrivateFieldGet(this, _data).dtlsRemoteCert;
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
     * Observer.
     *
     * @override
     * @emits close
     * @emits newproducer - (producer: Producer)
     * @emits newconsumer - (consumer: Consumer)
     * @emits newdataproducer - (dataProducer: DataProducer)
     * @emits newdataconsumer - (dataConsumer: DataConsumer)
     * @emits icestatechange - (iceState: IceState)
     * @emits iceselectedtuplechange - (iceSelectedTuple: TransportTuple)
     * @emits dtlsstatechange - (dtlsState: DtlsState)
     * @emits sctpstatechange - (sctpState: SctpState)
     * @emits trace - (trace: TransportTraceEventData)
     */
    // get observer(): EnhancedEventEmitter
    /**
     * Close the WebRtcTransport.
     *
     * @override
     */
    close() {
        if (this.closed)
            return;
        __classPrivateFieldGet(this, _data).iceState = 'closed';
        __classPrivateFieldGet(this, _data).iceSelectedTuple = undefined;
        __classPrivateFieldGet(this, _data).dtlsState = 'closed';
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
        __classPrivateFieldGet(this, _data).iceState = 'closed';
        __classPrivateFieldGet(this, _data).iceSelectedTuple = undefined;
        __classPrivateFieldGet(this, _data).dtlsState = 'closed';
        if (__classPrivateFieldGet(this, _data).sctpState)
            __classPrivateFieldGet(this, _data).sctpState = 'closed';
        super.routerClosed();
    }
    /**
     * Get WebRtcTransport stats.
     *
     * @override
     */
    async getStats() {
        logger.debug('getStats()');
        return this.channel.request('transport.getStats', this.internal);
    }
    /**
     * Provide the WebRtcTransport remote parameters.
     *
     * @override
     */
    async connect({ dtlsParameters }) {
        logger.debug('connect()');
        const reqData = { dtlsParameters };
        const data = await this.channel.request('transport.connect', this.internal, reqData);
        // Update data.
        __classPrivateFieldGet(this, _data).dtlsParameters.role = data.dtlsLocalRole;
    }
    /**
     * Restart ICE.
     */
    async restartIce() {
        logger.debug('restartIce()');
        const data = await this.channel.request('transport.restartIce', this.internal);
        const { iceParameters } = data;
        __classPrivateFieldGet(this, _data).iceParameters = iceParameters;
        return iceParameters;
    }
    handleWorkerNotifications() {
        this.channel.on(this.internal.transportId, (event, data) => {
            switch (event) {
                case 'icestatechange':
                    {
                        const iceState = data.iceState;
                        __classPrivateFieldGet(this, _data).iceState = iceState;
                        this.safeEmit('icestatechange', iceState);
                        // Emit observer event.
                        this.observer.safeEmit('icestatechange', iceState);
                        break;
                    }
                case 'iceselectedtuplechange':
                    {
                        const iceSelectedTuple = data.iceSelectedTuple;
                        __classPrivateFieldGet(this, _data).iceSelectedTuple = iceSelectedTuple;
                        this.safeEmit('iceselectedtuplechange', iceSelectedTuple);
                        // Emit observer event.
                        this.observer.safeEmit('iceselectedtuplechange', iceSelectedTuple);
                        break;
                    }
                case 'dtlsstatechange':
                    {
                        const dtlsState = data.dtlsState;
                        const dtlsRemoteCert = data.dtlsRemoteCert;
                        __classPrivateFieldGet(this, _data).dtlsState = dtlsState;
                        if (dtlsState === 'connected')
                            __classPrivateFieldGet(this, _data).dtlsRemoteCert = dtlsRemoteCert;
                        this.safeEmit('dtlsstatechange', dtlsState);
                        // Emit observer event.
                        this.observer.safeEmit('dtlsstatechange', dtlsState);
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
exports.WebRtcTransport = WebRtcTransport;
_data = new WeakMap();
