"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Logger_1 = require("./Logger");
const EnhancedEventEmitter_1 = require("./EnhancedEventEmitter");
const logger = new Logger_1.Logger('DataConsumer');
class DataConsumer extends EnhancedEventEmitter_1.EnhancedEventEmitter {
    /**
     * @private
     * @emits transportclose
     * @emits dataproducerclose
     * @emits @close
     * @emits @dataproducerclose
     */
    constructor({ internal, data, channel, appData }) {
        super();
        // Closed flag.
        this._closed = false;
        // Observer instance.
        this._observer = new EnhancedEventEmitter_1.EnhancedEventEmitter();
        logger.debug('constructor()');
        this._internal = internal;
        this._data = data;
        this._channel = channel;
        this._appData = appData;
        this._handleWorkerNotifications();
    }
    /**
     * DataConsumer id.
     */
    get id() {
        return this._internal.dataConsumerId;
    }
    /**
     * Associated DataProducer id.
     */
    get dataProducerId() {
        return this._internal.dataProducerId;
    }
    /**
     * Whether the DataConsumer is closed.
     */
    get closed() {
        return this._closed;
    }
    /**
     * SCTP stream parameters.
     */
    get sctpStreamParameters() {
        return this._data.sctpStreamParameters;
    }
    /**
     * DataChannel label.
     */
    get label() {
        return this._data.label;
    }
    /**
     * DataChannel protocol.
     */
    get protocol() {
        return this._data.protocol;
    }
    /**
     * App custom data.
     */
    get appData() {
        return this._appData;
    }
    /**
     * Invalid setter.
     */
    set appData(appData) {
        throw new Error('cannot override appData object');
    }
    /**
     * Observer.
     *
     * @emits close
     */
    get observer() {
        return this._observer;
    }
    /**
     * Close the DataConsumer.
     */
    close() {
        if (this._closed)
            return;
        logger.debug('close()');
        this._closed = true;
        // Remove notification subscriptions.
        this._channel.removeAllListeners(this._internal.dataConsumerId);
        this._channel.request('dataConsumer.close', this._internal)
            .catch(() => { });
        this.emit('@close');
        // Emit observer event.
        this._observer.safeEmit('close');
    }
    /**
     * Transport was closed.
     *
     * @private
     */
    transportClosed() {
        if (this._closed)
            return;
        logger.debug('transportClosed()');
        this._closed = true;
        // Remove notification subscriptions.
        this._channel.removeAllListeners(this._internal.dataConsumerId);
        this.safeEmit('transportclose');
        // Emit observer event.
        this._observer.safeEmit('close');
    }
    /**
     * Dump DataConsumer.
     */
    async dump() {
        logger.debug('dump()');
        return this._channel.request('dataConsumer.dump', this._internal);
    }
    /**
     * Get DataConsumer stats.
     */
    async getStats() {
        logger.debug('getStats()');
        return this._channel.request('dataConsumer.getStats', this._internal);
    }
    _handleWorkerNotifications() {
        this._channel.on(this._internal.dataConsumerId, (event) => {
            switch (event) {
                case 'dataproducerclose':
                    {
                        if (this._closed)
                            break;
                        this._closed = true;
                        // Remove notification subscriptions.
                        this._channel.removeAllListeners(this._internal.dataConsumerId);
                        this.emit('@dataproducerclose');
                        this.safeEmit('dataproducerclose');
                        // Emit observer event.
                        this._observer.safeEmit('close');
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
exports.DataConsumer = DataConsumer;
