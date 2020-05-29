"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Logger_1 = require("./Logger");
const errors_1 = require("./errors");
const Transport_1 = require("./Transport");
const logger = new Logger_1.Logger('DirectTransport');
class DirectTransport extends Transport_1.Transport {
    /**
     * @private
     * @emits trace - (trace: TransportTraceEventData)
     */
    constructor(params) {
        super(params);
        logger.debug('constructor()');
        const { data } = params;
        // TODO
        this._data = data;
        // {
        //
        // };
        this._handleWorkerNotifications();
    }
    /**
     * Observer.
     *
     * @override
     * @emits close
     * @emits newdataproducer - (dataProducer: DataProducer)
     * @emits newdataconsumer - (dataProducer: DataProducer)
     * @emits trace - (trace: TransportTraceEventData)
     */
    get observer() {
        return this._observer;
    }
    /**
     * Close the DirectTransport.
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
     * Get DirectTransport stats.
     *
     * @override
     */
    async getStats() {
        logger.debug('getStats()');
        return this._channel.request('transport.getStats', this._internal);
    }
    /**
     * NO-OP method in DirectTransport.
     *
     * @override
     */
    async connect() {
        logger.debug('connect()');
    }
    /**
     * @override
     */
    // eslint-disable-next-line @typescript-eslint/no-unused-vars
    async setMaxIncomingBitrate(bitrate) {
        throw new errors_1.UnsupportedError('setMaxIncomingBitrate() not implemented in DirectTransport');
    }
    /**
     * @override
     */
    // eslint-disable-next-line @typescript-eslint/no-unused-vars
    async produce(options) {
        throw new errors_1.UnsupportedError('produce() not implemented in DirectTransport');
    }
    /**
     * @override
     */
    // eslint-disable-next-line @typescript-eslint/no-unused-vars
    async consume(options) {
        throw new errors_1.UnsupportedError('consume() not implemented in DirectTransport');
    }
    _handleWorkerNotifications() {
        this._channel.on(this._internal.transportId, (event, data) => {
            switch (event) {
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
exports.DirectTransport = DirectTransport;
