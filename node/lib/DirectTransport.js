"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.DirectTransport = void 0;
const Logger_1 = require("./Logger");
const errors_1 = require("./errors");
const Transport_1 = require("./Transport");
const logger = new Logger_1.Logger('DirectTransport');
class DirectTransport extends Transport_1.Transport {
    // DirectTransport data.
    #data;
    /**
     * @private
     * @emits rtcp - (packet: Buffer)
     * @emits trace - (trace: TransportTraceEventData)
     */
    constructor(params) {
        super(params);
        logger.debug('constructor()');
        this.#data =
            {
            // Nothing for now.
            };
        this.handleWorkerNotifications();
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
    // get observer(): EnhancedEventEmitter
    /**
     * Close the DirectTransport.
     *
     * @override
     */
    close() {
        if (this.closed)
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
        if (this.closed)
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
        return this.channel.request('transport.getStats', this.internal);
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
    async setMaxOutgoingBitrate(bitrate) {
        throw new errors_1.UnsupportedError('setMaxOutgoingBitrate() not implemented in DirectTransport');
    }
    /**
     * Send RTCP packet.
     */
    sendRtcp(rtcpPacket) {
        if (!Buffer.isBuffer(rtcpPacket)) {
            throw new TypeError('rtcpPacket must be a Buffer');
        }
        this.payloadChannel.notify('transport.sendRtcp', this.internal, undefined, rtcpPacket);
    }
    handleWorkerNotifications() {
        this.channel.on(this.internal.transportId, (event, data) => {
            switch (event) {
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
        this.payloadChannel.on(this.internal.transportId, (event, data, payload) => {
            switch (event) {
                case 'rtcp':
                    {
                        if (this.closed)
                            break;
                        const packet = payload;
                        this.safeEmit('rtcp', packet);
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
