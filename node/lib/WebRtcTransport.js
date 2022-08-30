"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.WebRtcTransport = void 0;
const Logger_1 = require("./Logger");
const Transport_1 = require("./Transport");
const logger = new Logger_1.Logger('WebRtcTransport');
class WebRtcTransport extends Transport_1.Transport {
    // WebRtcTransport data.
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
            };
        this.handleWorkerNotifications();
    }
    /**
     * ICE role.
     */
    get iceRole() {
        return this.#data.iceRole;
    }
    /**
     * ICE parameters.
     */
    get iceParameters() {
        return this.#data.iceParameters;
    }
    /**
     * ICE candidates.
     */
    get iceCandidates() {
        return this.#data.iceCandidates;
    }
    /**
     * ICE state.
     */
    get iceState() {
        return this.#data.iceState;
    }
    /**
     * ICE selected tuple.
     */
    get iceSelectedTuple() {
        return this.#data.iceSelectedTuple;
    }
    /**
     * DTLS parameters.
     */
    get dtlsParameters() {
        return this.#data.dtlsParameters;
    }
    /**
     * DTLS state.
     */
    get dtlsState() {
        return this.#data.dtlsState;
    }
    /**
     * Remote certificate in PEM format.
     */
    get dtlsRemoteCert() {
        return this.#data.dtlsRemoteCert;
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
     * Close the WebRtcTransport.
     *
     * @override
     */
    close() {
        if (this.closed)
            return;
        this.#data.iceState = 'closed';
        this.#data.iceSelectedTuple = undefined;
        this.#data.dtlsState = 'closed';
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
        this.#data.iceState = 'closed';
        this.#data.iceSelectedTuple = undefined;
        this.#data.dtlsState = 'closed';
        if (this.#data.sctpState)
            this.#data.sctpState = 'closed';
        super.routerClosed();
    }
    /**
     * Called when closing the associated WebRtcServer.
     *
     * @private
     */
    webRtcServerClosed() {
        if (this.closed)
            return;
        this.#data.iceState = 'closed';
        this.#data.iceSelectedTuple = undefined;
        this.#data.dtlsState = 'closed';
        if (this.#data.sctpState)
            this.#data.sctpState = 'closed';
        super.listenServerClosed();
    }
    /**
     * Get WebRtcTransport stats.
     *
     * @override
     */
    async getStats() {
        logger.debug('getStats()');
        return this.channel.request('transport.getStats', this.internal.transportId);
    }
    /**
     * Provide the WebRtcTransport remote parameters.
     *
     * @override
     */
    async connect({ dtlsParameters }) {
        logger.debug('connect()');
        const reqData = { dtlsParameters };
        const data = await this.channel.request('transport.connect', this.internal.transportId, reqData);
        // Update data.
        this.#data.dtlsParameters.role = data.dtlsLocalRole;
    }
    /**
     * Restart ICE.
     */
    async restartIce() {
        logger.debug('restartIce()');
        const data = await this.channel.request('transport.restartIce', this.internal.transportId);
        const { iceParameters } = data;
        this.#data.iceParameters = iceParameters;
        return iceParameters;
    }
    handleWorkerNotifications() {
        this.channel.on(this.internal.transportId, (event, data) => {
            switch (event) {
                case 'icestatechange':
                    {
                        const iceState = data.iceState;
                        this.#data.iceState = iceState;
                        this.safeEmit('icestatechange', iceState);
                        // Emit observer event.
                        this.observer.safeEmit('icestatechange', iceState);
                        break;
                    }
                case 'iceselectedtuplechange':
                    {
                        const iceSelectedTuple = data.iceSelectedTuple;
                        this.#data.iceSelectedTuple = iceSelectedTuple;
                        this.safeEmit('iceselectedtuplechange', iceSelectedTuple);
                        // Emit observer event.
                        this.observer.safeEmit('iceselectedtuplechange', iceSelectedTuple);
                        break;
                    }
                case 'dtlsstatechange':
                    {
                        const dtlsState = data.dtlsState;
                        const dtlsRemoteCert = data.dtlsRemoteCert;
                        this.#data.dtlsState = dtlsState;
                        if (dtlsState === 'connected')
                            this.#data.dtlsRemoteCert = dtlsRemoteCert;
                        this.safeEmit('dtlsstatechange', dtlsState);
                        // Emit observer event.
                        this.observer.safeEmit('dtlsstatechange', dtlsState);
                        break;
                    }
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
exports.WebRtcTransport = WebRtcTransport;
