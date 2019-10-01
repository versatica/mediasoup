"use strict";
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const Logger_1 = __importDefault(require("./Logger"));
const Transport_1 = __importDefault(require("./Transport"));
const logger = new Logger_1.default('PlainRtpTransport');
class PlainRtpTransport extends Transport_1.default {
    // PlainRtpTransport data.
    // - .rtcpMux
    // - .comedia
    // - .multiSource
    // - .tuple
    //   - .localIp
    //   - .localPort
    //   - .remoteIp
    //   - .remotePort
    //   - .protocol
    // - .rtcpTuple
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
    /**
     * @private
     * @emits {sctpState: SctpState} sctpstatechange
     * @emits {TransportPacketEventData} packet
     */
    constructor(params) {
        super(params);
        logger.debug('constructor()');
        const { data } = params;
        this._data =
            {
                tuple: data.tuple,
                rtcpTuple: data.rtcpTuple,
                sctpParameters: data.sctpParameters,
                sctpState: data.sctpState
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
     * Transport RTCP tuple.
     */
    get rtcpTuple() {
        return this._data.rtcpTuple;
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
     * Observer.
     *
     * @override
     * @emits close
     * @emits {producer: Producer} newproducer
     * @emits {consumer: Consumer} newconsumer
     * @emits {producer: DataProducer} newdataproducer
     * @emits {consumer: DataConsumer} newdataconsumer
     * @emits {sctpState: SctpState} sctpstatechange
     * @emits {TransportPacketEventData} packet
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
     * Get PlainRtpTransport stats.
     *
     * @override
     */
    getStats() {
        return __awaiter(this, void 0, void 0, function* () {
            logger.debug('getStats()');
            return this._channel.request('transport.getStats', this._internal);
        });
    }
    /**
     * Provide the PlainRtpTransport remote parameters.
     *
     * @override
     */
    connect({ ip, port, rtcpPort }) {
        return __awaiter(this, void 0, void 0, function* () {
            logger.debug('connect()');
            const reqData = { ip, port, rtcpPort };
            const data = yield this._channel.request('transport.connect', this._internal, reqData);
            // Update data.
            this._data.tuple = data.tuple;
            this._data.rtcpTuple = data.rtcpTuple;
        });
    }
    /**
     * Override Transport.consume() method to reject it if multiSource is set.
     *
     * @override
     */
    consume(params) {
        const _super = Object.create(null, {
            consume: { get: () => super.consume }
        });
        return __awaiter(this, void 0, void 0, function* () {
            if (this._data.multiSource)
                throw new Error('cannot call consume() with multiSource set');
            return _super.consume.call(this, params);
        });
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
                case 'packet':
                    {
                        const packet = data;
                        this.safeEmit('packet', packet);
                        // Emit observer event.
                        this._observer.safeEmit('packet', packet);
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
exports.default = PlainRtpTransport;
