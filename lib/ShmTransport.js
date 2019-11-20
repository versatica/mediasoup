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
//import * as ortc from './ortc';
const Transport_1 = __importDefault(require("./Transport"));
const logger = new Logger_1.default('ShmTransport');
class ShmTransport extends Transport_1.default {
    /**
     * @private
     *
     * @emits {sctpState: String} sctpstatechange
     */
    constructor(params) {
        super(params);
        logger.debug('constructor()');
        const { data } = params;
        // ShmTransport data.
        // See sfushm_av_media.h for details
        // TODO: some may not be needed here, such as ssrc?
        // @type {Object}
        // - .shm
        //   - .name
        // - .log
        //   - .fileName (can be 'stdout' to redirect log output)
        //   - .level
        // - .channels[]
        //  - .target_buf_ms
        //  - .target_kpbs
        //  - .ssrc
        //  - .sample_rate
        //  - .num_audio_chn
        //  - .media_type (audio or video; TODO: smth to be done with rtcp and metadata?)
        // TODO: number of channels?
        this._data =
            {
                shm: data.shm,
                log: data.log,
                channels: data.channels
            };
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
     * Get PipeTransport stats.
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
     * Provide the ShmTransport remote parameters.
     *
     * @param {String} shm- shm name.
     *
     * @async
     * @override
     */
    connect({ shm }) {
        return __awaiter(this, void 0, void 0, function* () {
            logger.debug('connect()');
            const reqData = { shm };
            const data = yield this._channel.request('transport.connect', this._internal, reqData);
        });
    }
    /**
     *
     * @async
     * @override
     * @returns {Consumer}
     */
    consume({ producerId, appData = {} }) {
        const _super = Object.create(null, {
            consume: { get: () => super.consume }
        });
        return __awaiter(this, void 0, void 0, function* () {
            logger.debug('consume()');
            params.appData = {
                type = 'shm'
            };
            return _super.consume.call(this, { producerId, appData });
        });
    }
    /**
     * @private
     * @override
     */
    _handleWorkerNotifications() {
        return __awaiter(this, void 0, void 0, function* () {
            this._channel.on(this._internal.transportId, (event, data) => {
                switch (event) {
                    case 'writestreammetadata':
                        {
                            const reqdata = data;
                            const ret = yield this._channel.request('transport.writestreammetadata', this._internal, reqData); //TODO: so... let's write stream metadata via transport.writemetadata command
                            break;
                        }
                    default:
                        {
                            logger.error('ignoring unknown event "%s"', event);
                        }
                }
            });
        });
    }
}
exports.default = ShmTransport;
