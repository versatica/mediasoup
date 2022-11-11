"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.parseDirectTransportDump = exports.DirectTransport = void 0;
const Logger_1 = require("./Logger");
const errors_1 = require("./errors");
const Transport_1 = require("./Transport");
const FbsTransport = require("./fbs/transport_generated");
const FbsRequest = require("./fbs/request_generated");
const logger = new Logger_1.Logger('DirectTransport');
class DirectTransport extends Transport_1.Transport {
    // DirectTransport data.
    #data;
    /**
     * @private
     */
    constructor(options) {
        super(options);
        logger.debug('constructor()');
        this.#data =
            {
            // Nothing.
            };
        this.handleWorkerNotifications();
    }
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
     * Dump Transport.
     */
    async dump() {
        logger.debug('dump()');
        const response = await this.channel.requestBinary(FbsRequest.Method.TRANSPORT_DUMP, undefined, undefined, this.internal.transportId);
        /* Decode the response. */
        const dump = new FbsTransport.DumpResponse();
        response.body(dump);
        const transportDump = new FbsTransport.DirectTransportDump();
        dump.data(transportDump);
        return parseDirectTransportDump(transportDump);
    }
    /**
     * Get DirectTransport stats.
     *
     * @override
     */
    async getStats() {
        logger.debug('getStats()');
        return this.channel.request('transport.getStats', this.internal.transportId);
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
        this.payloadChannel.notify('transport.sendRtcp', this.internal.transportId, undefined, rtcpPacket);
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
function parseDirectTransportDump(binary) {
    // Retrieve BaseTransportDump.
    const fbsBaseTransportDump = new FbsTransport.BaseTransportDump();
    binary.base().data(fbsBaseTransportDump);
    return (0, Transport_1.parseBaseTransportDump)(fbsBaseTransportDump);
}
exports.parseDirectTransportDump = parseDirectTransportDump;
