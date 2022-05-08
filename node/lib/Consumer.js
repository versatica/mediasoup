"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.Consumer = void 0;
const Logger_1 = require("./Logger");
const EnhancedEventEmitter_1 = require("./EnhancedEventEmitter");
const logger = new Logger_1.Logger('Consumer');
class Consumer extends EnhancedEventEmitter_1.EnhancedEventEmitter {
    // Internal data.
    #internal;
    // Consumer data.
    #data;
    // Channel instance.
    #channel;
    // PayloadChannel instance.
    #payloadChannel;
    // Closed flag.
    #closed = false;
    // Custom app data.
    #appData;
    // Paused flag.
    #paused = false;
    // Associated Producer paused flag.
    #producerPaused = false;
    // Current priority.
    #priority = 1;
    // Current score.
    #score;
    // Preferred layers.
    #preferredLayers;
    // Curent layers.
    #currentLayers;
    // Observer instance.
    #observer = new EnhancedEventEmitter_1.EnhancedEventEmitter();
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
        logger.debug('constructor()');
        this.#internal = internal;
        this.#data = data;
        this.#channel = channel;
        this.#payloadChannel = payloadChannel;
        this.#appData = appData;
        this.#paused = paused;
        this.#producerPaused = producerPaused;
        this.#score = score;
        this.#preferredLayers = preferredLayers;
        this.handleWorkerNotifications();
    }
    /**
     * Consumer id.
     */
    get id() {
        return this.#internal.consumerId;
    }
    /**
     * Associated Producer id.
     */
    get producerId() {
        return this.#internal.producerId;
    }
    /**
     * Whether the Consumer is closed.
     */
    get closed() {
        return this.#closed;
    }
    /**
     * Media kind.
     */
    get kind() {
        return this.#data.kind;
    }
    /**
     * RTP parameters.
     */
    get rtpParameters() {
        return this.#data.rtpParameters;
    }
    /**
     * Consumer type.
     */
    get type() {
        return this.#data.type;
    }
    /**
     * Whether the Consumer is paused.
     */
    get paused() {
        return this.#paused;
    }
    /**
     * Whether the associate Producer is paused.
     */
    get producerPaused() {
        return this.#producerPaused;
    }
    /**
     * Current priority.
     */
    get priority() {
        return this.#priority;
    }
    /**
     * Consumer score.
     */
    get score() {
        return this.#score;
    }
    /**
     * Preferred video layers.
     */
    get preferredLayers() {
        return this.#preferredLayers;
    }
    /**
     * Current video layers.
     */
    get currentLayers() {
        return this.#currentLayers;
    }
    /**
     * App custom data.
     */
    get appData() {
        return this.#appData;
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
        return this.#observer;
    }
    /**
     * @private
     * Just for testing purposes.
     */
    get channelForTesting() {
        return this.#channel;
    }
    /**
     * Close the Consumer.
     */
    close() {
        if (this.#closed)
            return;
        logger.debug('close()');
        this.#closed = true;
        // Remove notification subscriptions.
        this.#channel.removeAllListeners(this.#internal.consumerId);
        this.#payloadChannel.removeAllListeners(this.#internal.consumerId);
        this.#channel.request('consumer.close', this.#internal)
            .catch(() => { });
        this.emit('@close');
        // Emit observer event.
        this.#observer.safeEmit('close');
    }
    /**
     * Transport was closed.
     *
     * @private
     */
    transportClosed() {
        if (this.#closed)
            return;
        logger.debug('transportClosed()');
        this.#closed = true;
        // Remove notification subscriptions.
        this.#channel.removeAllListeners(this.#internal.consumerId);
        this.#payloadChannel.removeAllListeners(this.#internal.consumerId);
        this.safeEmit('transportclose');
        // Emit observer event.
        this.#observer.safeEmit('close');
    }
    /**
     * Dump Consumer.
     */
    async dump() {
        logger.debug('dump()');
        return this.#channel.request('consumer.dump', this.#internal);
    }
    /**
     * Get Consumer stats.
     */
    async getStats() {
        logger.debug('getStats()');
        return this.#channel.request('consumer.getStats', this.#internal);
    }
    /**
     * Pause the Consumer.
     */
    async pause() {
        logger.debug('pause()');
        const wasPaused = this.#paused || this.#producerPaused;
        await this.#channel.request('consumer.pause', this.#internal);
        this.#paused = true;
        // Emit observer event.
        if (!wasPaused)
            this.#observer.safeEmit('pause');
    }
    /**
     * Resume the Consumer.
     */
    async resume() {
        logger.debug('resume()');
        const wasPaused = this.#paused || this.#producerPaused;
        await this.#channel.request('consumer.resume', this.#internal);
        this.#paused = false;
        // Emit observer event.
        if (wasPaused && !this.#producerPaused)
            this.#observer.safeEmit('resume');
    }
    /**
     * Set preferred video layers.
     */
    async setPreferredLayers({ spatialLayer, temporalLayer }) {
        logger.debug('setPreferredLayers()');
        const reqData = { spatialLayer, temporalLayer };
        const data = await this.#channel.request('consumer.setPreferredLayers', this.#internal, reqData);
        this.#preferredLayers = data || undefined;
    }
    /**
     * Set priority.
     */
    async setPriority(priority) {
        logger.debug('setPriority()');
        const reqData = { priority };
        const data = await this.#channel.request('consumer.setPriority', this.#internal, reqData);
        this.#priority = data.priority;
    }
    /**
     * Unset priority.
     */
    async unsetPriority() {
        logger.debug('unsetPriority()');
        const reqData = { priority: 1 };
        const data = await this.#channel.request('consumer.setPriority', this.#internal, reqData);
        this.#priority = data.priority;
    }
    /**
     * Request a key frame to the Producer.
     */
    async requestKeyFrame() {
        logger.debug('requestKeyFrame()');
        await this.#channel.request('consumer.requestKeyFrame', this.#internal);
    }
    /**
     * Enable 'trace' event.
     */
    async enableTraceEvent(types = []) {
        logger.debug('enableTraceEvent()');
        const reqData = { types };
        await this.#channel.request('consumer.enableTraceEvent', this.#internal, reqData);
    }
    handleWorkerNotifications() {
        this.#channel.on(this.#internal.consumerId, (event, data) => {
            switch (event) {
                case 'producerclose':
                    {
                        if (this.#closed)
                            break;
                        this.#closed = true;
                        // Remove notification subscriptions.
                        this.#channel.removeAllListeners(this.#internal.consumerId);
                        this.#payloadChannel.removeAllListeners(this.#internal.consumerId);
                        this.emit('@producerclose');
                        this.safeEmit('producerclose');
                        // Emit observer event.
                        this.#observer.safeEmit('close');
                        break;
                    }
                case 'producerpause':
                    {
                        if (this.#producerPaused)
                            break;
                        const wasPaused = this.#paused || this.#producerPaused;
                        this.#producerPaused = true;
                        this.safeEmit('producerpause');
                        // Emit observer event.
                        if (!wasPaused)
                            this.#observer.safeEmit('pause');
                        break;
                    }
                case 'producerresume':
                    {
                        if (!this.#producerPaused)
                            break;
                        const wasPaused = this.#paused || this.#producerPaused;
                        this.#producerPaused = false;
                        this.safeEmit('producerresume');
                        // Emit observer event.
                        if (wasPaused && !this.#paused)
                            this.#observer.safeEmit('resume');
                        break;
                    }
                case 'score':
                    {
                        const score = data;
                        this.#score = score;
                        this.safeEmit('score', score);
                        // Emit observer event.
                        this.#observer.safeEmit('score', score);
                        break;
                    }
                case 'layerschange':
                    {
                        const layers = data;
                        this.#currentLayers = layers;
                        this.safeEmit('layerschange', layers);
                        // Emit observer event.
                        this.#observer.safeEmit('layerschange', layers);
                        break;
                    }
                case 'trace':
                    {
                        const trace = data;
                        this.safeEmit('trace', trace);
                        // Emit observer event.
                        this.#observer.safeEmit('trace', trace);
                        break;
                    }
                default:
                    {
                        logger.error('ignoring unknown event "%s"', event);
                    }
            }
        });
        this.#payloadChannel.on(this.#internal.consumerId, (event, data, payload) => {
            switch (event) {
                case 'rtp':
                    {
                        if (this.#closed)
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
