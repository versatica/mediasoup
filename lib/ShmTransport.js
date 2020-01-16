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
Object.defineProperty(exports, "__esModule", { value: true });
const Logger_1 = require("./Logger");
//import * as ortc from './ortc';
const Transport_1 = require("./Transport");
const logger = new Logger_1.Logger('ShmTransport');
class ShmTransport extends Transport_1.Transport {
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
        // @type {Object}
        // - .shm
        //   - .name
        // - .log
        //   - .fileName (can be 'stdout' to redirect log output)
        //   - .level
        this._data =
            {
                shm: data.shm,
                log: data.log
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
            yield this._channel.request('transport.connect', this._internal, reqData);
        });
    }
    /**
     * @private
     * @override
     */
    _handleWorkerNotifications() {
        return __awaiter(this, void 0, void 0, function* () {
            this._channel.on(this._internal.transportId, (event, data) => __awaiter(this, void 0, void 0, function* () {
                switch (event) {
                    case 'writestreammetadata':
                        {
                            const reqdata = {
                                shm: this._data.shm,
                                meta: data
                            }; // TODO: assume there is shm name and some JSON object, no details yet
                            yield this._channel.request('transport.consumeStreamMeta', this._internal, reqdata);
                            break;
                        }
                    default:
                        {
                            logger.error('ignoring unknown event "%s"', event);
                        }
                }
            }));
        });
    }
}
exports.ShmTransport = ShmTransport;
