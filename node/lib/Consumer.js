"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Logger_1 = require("./Logger");
const EnhancedEventEmitter_1 = require("./EnhancedEventEmitter");
const logger = new Logger_1.Logger('Consumer');
class Consumer extends EnhancedEventEmitter_1.EnhancedEventEmitter {
    /**
     * @private
     * @emits transportclose
     * @emits producerclose
     * @emits producerpause
     * @emits producerresume
     * @emits score - (score: ConsumerScore)
     * @emits layerschange - (layers: ConsumerLayers | undefined)
     * @emits rtp - (packet: Buffer)
     * @emits trace - (trace: ConsumerTraceEventData)
     * @emits @close
     * @emits @producerclose
     */
    constructor({ internal, data, channel, payloadChannel, appData, paused, producerPaused, score = { score: 10, producerScore: 10, producerScores: [] }, preferredLayers }) {
        super();
        // Closed flag.
        this._closed = false;
        // Paused flag.
        this._paused = false;
        // Associated Producer paused flag.
        this._producerPaused = false;
        // Current priority.
        this._priority = 1;
        // Observer instance.
        this._observer = new EnhancedEventEmitter_1.EnhancedEventEmitter();
        logger.debug('constructor()');
        this._internal = internal;
        this._data = data;
        this._channel = channel;
        this._payloadChannel = payloadChannel;
        this._appData = appData;
        this._paused = paused;
        this._producerPaused = producerPaused;
        this._score = score;
        this._preferredLayers = preferredLayers;
        this._handleWorkerNotifications();
    }
    /**
     * Consumer id.
     */
    get id() {
        return this._internal.consumerId;
    }
    /**
     * Associated Producer id.
     */
    get producerId() {
        return this._internal.producerId;
    }
    /**
     * Whether the Consumer is closed.
     */
    get closed() {
        return this._closed;
    }
    /**
     * Media kind.
     */
    get kind() {
        return this._data.kind;
    }
    /**
     * RTP parameters.
     */
    get rtpParameters() {
        return this._data.rtpParameters;
    }
    /**
     * Consumer type.
     */
    get type() {
        return this._data.type;
    }
    /**
     * Whether the Consumer is paused.
     */
    get paused() {
        return this._paused;
    }
    /**
     * Whether the associate Producer is paused.
     */
    get producerPaused() {
        return this._producerPaused;
    }
    /**
     * Current priority.
     */
    get priority() {
        return this._priority;
    }
    /**
     * Consumer score.
     */
    get score() {
        return this._score;
    }
    /**
     * Preferred video layers.
     */
    get preferredLayers() {
        return this._preferredLayers;
    }
    /**
     * Current video layers.
     */
    get currentLayers() {
        return this._currentLayers;
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
     * @emits pause
     * @emits resume
     * @emits score - (score: ConsumerScore)
     * @emits layerschange - (layers: ConsumerLayers | undefined)
     * @emits trace - (trace: ConsumerTraceEventData)
     */
    get observer() {
        return this._observer;
    }
    /**
     * Close the Consumer.
     */
    close() {
        if (this._closed)
            return;
        logger.debug('close()');
        this._closed = true;
        // Remove notification subscriptions.
        this._channel.removeAllListeners(this._internal.consumerId);
        this._payloadChannel.removeAllListeners(this._internal.consumerId);
        this._channel.request('consumer.close', this._internal)
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
        this._channel.removeAllListeners(this._internal.consumerId);
        this._payloadChannel.removeAllListeners(this._internal.consumerId);
        this.safeEmit('transportclose');
        // Emit observer event.
        this._observer.safeEmit('close');
    }
    /**
     * Dump Consumer.
     */
    async dump() {
        logger.debug('dump()');
        return this._channel.request('consumer.dump', this._internal);
    }
    /**
     * Get Consumer stats.
     */
    async getStats() {
        logger.debug('getStats()');
        return this._channel.request('consumer.getStats', this._internal);
    }
    /**
     * Pause the Consumer.
     */
    async pause() {
        logger.debug('pause()');
        const wasPaused = this._paused || this._producerPaused;
        await this._channel.request('consumer.pause', this._internal);
        this._paused = true;
        // Emit observer event.
        if (!wasPaused)
            this._observer.safeEmit('pause');
    }
    /**
     * Resume the Consumer.
     */
    async resume() {
        logger.debug('resume()');
        const wasPaused = this._paused || this._producerPaused;
        await this._channel.request('consumer.resume', this._internal);
        this._paused = false;
        // Emit observer event.
        if (wasPaused && !this._producerPaused)
            this._observer.safeEmit('resume');
    }
    /**
     * Set preferred video layers.
     */
    async setPreferredLayers({ spatialLayer, temporalLayer }) {
        logger.debug('setPreferredLayers()');
        const reqData = { spatialLayer, temporalLayer };
        const data = await this._channel.request('consumer.setPreferredLayers', this._internal, reqData);
        this._preferredLayers = data || undefined;
    }
    /**
     * Set priority.
     */
    async setPriority(priority) {
        logger.debug('setPriority()');
        const reqData = { priority };
        const data = await this._channel.request('consumer.setPriority', this._internal, reqData);
        this._priority = data.priority;
    }
    /**
     * Unset priority.
     */
    async unsetPriority() {
        logger.debug('unsetPriority()');
        const reqData = { priority: 1 };
        const data = await this._channel.request('consumer.setPriority', this._internal, reqData);
        this._priority = data.priority;
    }
    /**
     * Request a key frame to the Producer.
     */
    async requestKeyFrame() {
        logger.debug('requestKeyFrame()');
        await this._channel.request('consumer.requestKeyFrame', this._internal);
    }
    /**
     * Enable 'trace' event.
     */
    async enableTraceEvent(types = []) {
        logger.debug('enableTraceEvent()');
        const reqData = { types };
        await this._channel.request('consumer.enableTraceEvent', this._internal, reqData);
    }
    _handleWorkerNotifications() {
        this._channel.on(this._internal.consumerId, (event, data) => {
            switch (event) {
                case 'producerclose':
                    {
                        if (this._closed)
                            break;
                        this._closed = true;
                        // Remove notification subscriptions.
                        this._channel.removeAllListeners(this._internal.consumerId);
                        this._payloadChannel.removeAllListeners(this._internal.consumerId);
                        this.emit('@producerclose');
                        this.safeEmit('producerclose');
                        // Emit observer event.
                        this._observer.safeEmit('close');
                        break;
                    }
                case 'producerpause':
                    {
                        if (this._producerPaused)
                            break;
                        const wasPaused = this._paused || this._producerPaused;
                        this._producerPaused = true;
                        this.safeEmit('producerpause');
                        // Emit observer event.
                        if (!wasPaused)
                            this._observer.safeEmit('pause');
                        break;
                    }
                case 'producerresume':
                    {
                        if (!this._producerPaused)
                            break;
                        const wasPaused = this._paused || this._producerPaused;
                        this._producerPaused = false;
                        this.safeEmit('producerresume');
                        // Emit observer event.
                        if (wasPaused && !this._paused)
                            this._observer.safeEmit('resume');
                        break;
                    }
                case 'score':
                    {
                        const score = data;
                        this._score = score;
                        this.safeEmit('score', score);
                        // Emit observer event.
                        this._observer.safeEmit('score', score);
                        break;
                    }
                case 'layerschange':
                    {
                        const layers = data;
                        this._currentLayers = layers;
                        this.safeEmit('layerschange', layers);
                        // Emit observer event.
                        this._observer.safeEmit('layerschange', layers);
                        break;
                    }
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
        this._payloadChannel.on(this._internal.consumerId, (event, data, payload) => {
            switch (event) {
                case 'rtp':
                    {
                        if (this._closed)
                            break;
                        const packet = payload;
                        this.safeEmit('rtp', packet);
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
exports.Consumer = Consumer;
