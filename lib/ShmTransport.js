"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Logger_1 = require("./Logger");
//import * as ortc from './ortc';
const Transport_1 = require("./Transport");
const logger = new Logger_1.Logger('ShmTransport');
class ShmTransport extends Transport_1.Transport {
    /**
    * @private
     *
     */
    constructor(params) {
        super(params);
        logger.debug('constructor()');
        const { data } = params;
        // ShmTransport data.
        this._shm = data.shm.name;
        this._log = data.shm.log;
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
     * Get ShmTransport stats.
     *
     * @override
     */
    async getStats() {
        logger.debug('getStats()');
        return this._channel.request('transport.getStats', this._internal);
    }
    /**
     * Provide the ShmTransport remote parameters.
     *
     * @param {String} shm- shm name.
     *
     * @async
     * @override
     */
    async connect({ shm }) {
        logger.debug('connect()');
        const reqData = { shm };
        await this._channel.request('transport.connect', this._internal, reqData);
    }
    /**
     * Provide the ShmTransport remote parameters.
     *
     * @param {Object} meta - metadata string.
     *
     * @async
     */
    async writeStreamMetaData({ meta }) {
        logger.debug('writeStreamMetaData()');
        const reqData = {
            meta,
            shm: this._shm,
            log: this._log
        };
        await this._channel.request('transport.consumeStreamMeta', this._internal, reqData);
    }
    /**
     * Does nothing, should not be called like this
     *
     * @private
     * @override
     */
    _handleWorkerNotifications() {
        this._channel.on(this._internal.transportId, async (event, data) => {
            switch (event) {
                default:
                    {
                        logger.error('ignoring unknown event "%s"', event);
                    }
            }
        });
    }
}
exports.ShmTransport = ShmTransport;
