"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.Consumer = void 0;
const Logger_1 = require("./Logger");
const EnhancedEventEmitter_1 = require("./EnhancedEventEmitter");
const FbsRequest = require("./fbs/request_generated");
const FbsTransport = require("./fbs/transport_generated");
const FbsConsumer = require("./fbs/consumer_generated");
const FbsCommon = require("./fbs/common_generated");
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
     */
    constructor({ internal, data, channel, payloadChannel, appData, paused, producerPaused, score = { score: 10, producerScore: 10, producerScores: [] }, preferredLayers }) {
        super();
        logger.debug('constructor()');
        this.#internal = internal;
        this.#data = data;
        this.#channel = channel;
        this.#payloadChannel = payloadChannel;
        this.#appData = appData || {};
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
        return this.#data.producerId;
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
        /* Build Request. */
        const builder = this.#channel.bufferBuilder;
        const requestOffset = new FbsTransport.CloseConsumerRequestT(this.#internal.consumerId).pack(builder);
        this.#channel.requestBinary(FbsRequest.Method.TRANSPORT_CLOSE_CONSUMER, FbsRequest.Body.FBS_Transport_CloseConsumerRequest, requestOffset, this.#internal.transportId).catch(() => { });
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
        return this.#channel.request('consumer.dump', this.#internal.consumerId);
    }
    /**
     * Get Consumer stats.
     */
    async getStats() {
        logger.debug('getStats()');
        return this.#channel.request('consumer.getStats', this.#internal.consumerId);
    }
    /**
     * Pause the Consumer.
     */
    async pause() {
        logger.debug('pause()');
        const wasPaused = this.#paused || this.#producerPaused;
        await this.#channel.requestBinary(FbsRequest.Method.CONSUMER_PAUSE, undefined, undefined, this.#internal.consumerId);
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
        await this.#channel.requestBinary(FbsRequest.Method.CONSUMER_RESUME, undefined, undefined, this.#internal.consumerId);
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
        const builder = this.#channel.bufferBuilder;
        let temporalLayerOffset = 0;
        // temporalLayer is optional.
        if (temporalLayer) {
            temporalLayerOffset = FbsCommon.OptionalInt16.createOptionalInt16(builder, temporalLayer);
        }
        FbsConsumer.ConsumerLayers.startConsumerLayers(builder);
        FbsConsumer.ConsumerLayers.addSpatialLayer(builder, spatialLayer);
        FbsConsumer.ConsumerLayers.addTemporalLayer(builder, temporalLayerOffset);
        const preferredLayersOffset = FbsConsumer.ConsumerLayers.endConsumerLayers(builder);
        const requestOffset = FbsConsumer.SetPreferredLayersRequest.createSetPreferredLayersRequest(builder, preferredLayersOffset);
        const response = await this.#channel.requestBinary(FbsRequest.Method.CONSUMER_SET_PREFERRED_LAYERS, FbsRequest.Body.FBS_Consumer_SetPreferredLayersRequest, requestOffset, this.#internal.consumerId);
        /* Decode the response. */
        const data = new FbsConsumer.SetPreferredLayersResponse();
        let preferredLayers;
        // Response is empty for non Simulcast Consumers.
        if (response.body(data)) {
            const status = data.unpack();
            if (status.preferredLayers) {
                preferredLayers = {
                    spatialLayer: status.preferredLayers.spatialLayer,
                    temporalLayer: status.preferredLayers.temporalLayer ?
                        status.preferredLayers.temporalLayer.value :
                        undefined
                };
            }
        }
        this.#preferredLayers = preferredLayers;
    }
    /**
     * Set priority.
     */
    async setPriority(priority) {
        logger.debug('setPriority()');
        const reqData = { priority };
        const data = await this.#channel.request('consumer.setPriority', this.#internal.consumerId, reqData);
        this.#priority = data.priority;
    }
    /**
     * Unset priority.
     */
    async unsetPriority() {
        logger.debug('unsetPriority()');
        const reqData = { priority: 1 };
        const data = await this.#channel.request('consumer.setPriority', this.#internal.consumerId, reqData);
        this.#priority = data.priority;
    }
    /**
     * Request a key frame to the Producer.
     */
    async requestKeyFrame() {
        logger.debug('requestKeyFrame()');
        await this.#channel.requestBinary(FbsRequest.Method.CONSUMER_REQUEST_KEY_FRAME, undefined, undefined, this.#internal.consumerId);
    }
    /**
     * Enable 'trace' event.
     */
    async enableTraceEvent(types = []) {
        logger.debug('enableTraceEvent()');
        if (!Array.isArray(types))
            throw new TypeError('types must be an array');
        if (types.find((type) => typeof type !== 'string'))
            throw new TypeError('every type must be a string');
        /* Build Request. */
        const builder = this.#channel.bufferBuilder;
        const requestOffset = new FbsConsumer.EnableTraceEventRequestT(types).pack(builder);
        await this.#channel.requestBinary(FbsRequest.Method.CONSUMER_ENABLE_TRACE_EVENT, FbsRequest.Body.FBS_Consumer_EnableTraceEventRequest, requestOffset, this.#internal.consumerId);
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
