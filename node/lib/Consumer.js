"use strict";
var __classPrivateFieldSet = (this && this.__classPrivateFieldSet) || function (receiver, privateMap, value) {
    if (!privateMap.has(receiver)) {
        throw new TypeError("attempted to set private field on non-instance");
    }
    privateMap.set(receiver, value);
    return value;
};
var __classPrivateFieldGet = (this && this.__classPrivateFieldGet) || function (receiver, privateMap) {
    if (!privateMap.has(receiver)) {
        throw new TypeError("attempted to get private field on non-instance");
    }
    return privateMap.get(receiver);
};
var _internal, _data, _channel, _payloadChannel, _closed, _appData, _paused, _producerPaused, _priority, _score, _preferredLayers, _currentLayers, _observer;
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
        // Internal data.
        _internal.set(this, void 0);
        // Consumer data.
        _data.set(this, void 0);
        // Channel instance.
        _channel.set(this, void 0);
        // PayloadChannel instance.
        _payloadChannel.set(this, void 0);
        // Closed flag.
        _closed.set(this, false);
        // Custom app data.
        _appData.set(this, void 0);
        // Paused flag.
        _paused.set(this, false);
        // Associated Producer paused flag.
        _producerPaused.set(this, false);
        // Current priority.
        _priority.set(this, 1);
        // Current score.
        _score.set(this, void 0);
        // Preferred layers.
        _preferredLayers.set(this, void 0);
        // Curent layers.
        _currentLayers.set(this, void 0);
        // Observer instance.
        _observer.set(this, new EnhancedEventEmitter_1.EnhancedEventEmitter());
        logger.debug('constructor()');
        __classPrivateFieldSet(this, _internal, internal);
        __classPrivateFieldSet(this, _data, data);
        __classPrivateFieldSet(this, _channel, channel);
        __classPrivateFieldSet(this, _payloadChannel, payloadChannel);
        __classPrivateFieldSet(this, _appData, appData);
        __classPrivateFieldSet(this, _paused, paused);
        __classPrivateFieldSet(this, _producerPaused, producerPaused);
        __classPrivateFieldSet(this, _score, score);
        __classPrivateFieldSet(this, _preferredLayers, preferredLayers);
        this.handleWorkerNotifications();
    }
    /**
     * Consumer id.
     */
    get id() {
        return __classPrivateFieldGet(this, _internal).consumerId;
    }
    /**
     * Associated Producer id.
     */
    get producerId() {
        return __classPrivateFieldGet(this, _internal).producerId;
    }
    /**
     * Whether the Consumer is closed.
     */
    get closed() {
        return __classPrivateFieldGet(this, _closed);
    }
    /**
     * Media kind.
     */
    get kind() {
        return __classPrivateFieldGet(this, _data).kind;
    }
    /**
     * RTP parameters.
     */
    get rtpParameters() {
        return __classPrivateFieldGet(this, _data).rtpParameters;
    }
    /**
     * Consumer type.
     */
    get type() {
        return __classPrivateFieldGet(this, _data).type;
    }
    /**
     * Whether the Consumer is paused.
     */
    get paused() {
        return __classPrivateFieldGet(this, _paused);
    }
    /**
     * Whether the associate Producer is paused.
     */
    get producerPaused() {
        return __classPrivateFieldGet(this, _producerPaused);
    }
    /**
     * Current priority.
     */
    get priority() {
        return __classPrivateFieldGet(this, _priority);
    }
    /**
     * Consumer score.
     */
    get score() {
        return __classPrivateFieldGet(this, _score);
    }
    /**
     * Preferred video layers.
     */
    get preferredLayers() {
        return __classPrivateFieldGet(this, _preferredLayers);
    }
    /**
     * Current video layers.
     */
    get currentLayers() {
        return __classPrivateFieldGet(this, _currentLayers);
    }
    /**
     * App custom data.
     */
    get appData() {
        return __classPrivateFieldGet(this, _appData);
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
        return __classPrivateFieldGet(this, _observer);
    }
    /**
     * @private
     * Just for testing purposes.
     */
    get channelForTesting() {
        return __classPrivateFieldGet(this, _channel);
    }
    /**
     * Close the Consumer.
     */
    close() {
        if (__classPrivateFieldGet(this, _closed))
            return;
        logger.debug('close()');
        __classPrivateFieldSet(this, _closed, true);
        // Remove notification subscriptions.
        __classPrivateFieldGet(this, _channel).removeAllListeners(__classPrivateFieldGet(this, _internal).consumerId);
        __classPrivateFieldGet(this, _payloadChannel).removeAllListeners(__classPrivateFieldGet(this, _internal).consumerId);
        __classPrivateFieldGet(this, _channel).request('consumer.close', __classPrivateFieldGet(this, _internal))
            .catch(() => { });
        this.emit('@close');
        // Emit observer event.
        __classPrivateFieldGet(this, _observer).safeEmit('close');
    }
    /**
     * Transport was closed.
     *
     * @private
     */
    transportClosed() {
        if (__classPrivateFieldGet(this, _closed))
            return;
        logger.debug('transportClosed()');
        __classPrivateFieldSet(this, _closed, true);
        // Remove notification subscriptions.
        __classPrivateFieldGet(this, _channel).removeAllListeners(__classPrivateFieldGet(this, _internal).consumerId);
        __classPrivateFieldGet(this, _payloadChannel).removeAllListeners(__classPrivateFieldGet(this, _internal).consumerId);
        this.safeEmit('transportclose');
        // Emit observer event.
        __classPrivateFieldGet(this, _observer).safeEmit('close');
    }
    /**
     * Dump Consumer.
     */
    async dump() {
        logger.debug('dump()');
        return __classPrivateFieldGet(this, _channel).request('consumer.dump', __classPrivateFieldGet(this, _internal));
    }
    /**
     * Get Consumer stats.
     */
    async getStats() {
        logger.debug('getStats()');
        return __classPrivateFieldGet(this, _channel).request('consumer.getStats', __classPrivateFieldGet(this, _internal));
    }
    /**
     * Pause the Consumer.
     */
    async pause() {
        logger.debug('pause()');
        const wasPaused = __classPrivateFieldGet(this, _paused) || __classPrivateFieldGet(this, _producerPaused);
        await __classPrivateFieldGet(this, _channel).request('consumer.pause', __classPrivateFieldGet(this, _internal));
        __classPrivateFieldSet(this, _paused, true);
        // Emit observer event.
        if (!wasPaused)
            __classPrivateFieldGet(this, _observer).safeEmit('pause');
    }
    /**
     * Resume the Consumer.
     */
    async resume() {
        logger.debug('resume()');
        const wasPaused = __classPrivateFieldGet(this, _paused) || __classPrivateFieldGet(this, _producerPaused);
        await __classPrivateFieldGet(this, _channel).request('consumer.resume', __classPrivateFieldGet(this, _internal));
        __classPrivateFieldSet(this, _paused, false);
        // Emit observer event.
        if (wasPaused && !__classPrivateFieldGet(this, _producerPaused))
            __classPrivateFieldGet(this, _observer).safeEmit('resume');
    }
    /**
     * Set preferred video layers.
     */
    async setPreferredLayers({ spatialLayer, temporalLayer }) {
        logger.debug('setPreferredLayers()');
        const reqData = { spatialLayer, temporalLayer };
        const data = await __classPrivateFieldGet(this, _channel).request('consumer.setPreferredLayers', __classPrivateFieldGet(this, _internal), reqData);
        __classPrivateFieldSet(this, _preferredLayers, data || undefined);
    }
    /**
     * Set priority.
     */
    async setPriority(priority) {
        logger.debug('setPriority()');
        const reqData = { priority };
        const data = await __classPrivateFieldGet(this, _channel).request('consumer.setPriority', __classPrivateFieldGet(this, _internal), reqData);
        __classPrivateFieldSet(this, _priority, data.priority);
    }
    /**
     * Unset priority.
     */
    async unsetPriority() {
        logger.debug('unsetPriority()');
        const reqData = { priority: 1 };
        const data = await __classPrivateFieldGet(this, _channel).request('consumer.setPriority', __classPrivateFieldGet(this, _internal), reqData);
        __classPrivateFieldSet(this, _priority, data.priority);
    }
    /**
     * Request a key frame to the Producer.
     */
    async requestKeyFrame() {
        logger.debug('requestKeyFrame()');
        await __classPrivateFieldGet(this, _channel).request('consumer.requestKeyFrame', __classPrivateFieldGet(this, _internal));
    }
    /**
     * Enable 'trace' event.
     */
    async enableTraceEvent(types = []) {
        logger.debug('enableTraceEvent()');
        const reqData = { types };
        await __classPrivateFieldGet(this, _channel).request('consumer.enableTraceEvent', __classPrivateFieldGet(this, _internal), reqData);
    }
    handleWorkerNotifications() {
        __classPrivateFieldGet(this, _channel).on(__classPrivateFieldGet(this, _internal).consumerId, (event, data) => {
            switch (event) {
                case 'producerclose':
                    {
                        if (__classPrivateFieldGet(this, _closed))
                            break;
                        __classPrivateFieldSet(this, _closed, true);
                        // Remove notification subscriptions.
                        __classPrivateFieldGet(this, _channel).removeAllListeners(__classPrivateFieldGet(this, _internal).consumerId);
                        __classPrivateFieldGet(this, _payloadChannel).removeAllListeners(__classPrivateFieldGet(this, _internal).consumerId);
                        this.emit('@producerclose');
                        this.safeEmit('producerclose');
                        // Emit observer event.
                        __classPrivateFieldGet(this, _observer).safeEmit('close');
                        break;
                    }
                case 'producerpause':
                    {
                        if (__classPrivateFieldGet(this, _producerPaused))
                            break;
                        const wasPaused = __classPrivateFieldGet(this, _paused) || __classPrivateFieldGet(this, _producerPaused);
                        __classPrivateFieldSet(this, _producerPaused, true);
                        this.safeEmit('producerpause');
                        // Emit observer event.
                        if (!wasPaused)
                            __classPrivateFieldGet(this, _observer).safeEmit('pause');
                        break;
                    }
                case 'producerresume':
                    {
                        if (!__classPrivateFieldGet(this, _producerPaused))
                            break;
                        const wasPaused = __classPrivateFieldGet(this, _paused) || __classPrivateFieldGet(this, _producerPaused);
                        __classPrivateFieldSet(this, _producerPaused, false);
                        this.safeEmit('producerresume');
                        // Emit observer event.
                        if (wasPaused && !__classPrivateFieldGet(this, _paused))
                            __classPrivateFieldGet(this, _observer).safeEmit('resume');
                        break;
                    }
                case 'score':
                    {
                        const score = data;
                        __classPrivateFieldSet(this, _score, score);
                        this.safeEmit('score', score);
                        // Emit observer event.
                        __classPrivateFieldGet(this, _observer).safeEmit('score', score);
                        break;
                    }
                case 'layerschange':
                    {
                        const layers = data;
                        __classPrivateFieldSet(this, _currentLayers, layers);
                        this.safeEmit('layerschange', layers);
                        // Emit observer event.
                        __classPrivateFieldGet(this, _observer).safeEmit('layerschange', layers);
                        break;
                    }
                case 'trace':
                    {
                        const trace = data;
                        this.safeEmit('trace', trace);
                        // Emit observer event.
                        __classPrivateFieldGet(this, _observer).safeEmit('trace', trace);
                        break;
                    }
                default:
                    {
                        logger.error('ignoring unknown event "%s"', event);
                    }
            }
        });
        __classPrivateFieldGet(this, _payloadChannel).on(__classPrivateFieldGet(this, _internal).consumerId, (event, data, payload) => {
            switch (event) {
                case 'rtp':
                    {
                        if (__classPrivateFieldGet(this, _closed))
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
_internal = new WeakMap(), _data = new WeakMap(), _channel = new WeakMap(), _payloadChannel = new WeakMap(), _closed = new WeakMap(), _appData = new WeakMap(), _paused = new WeakMap(), _producerPaused = new WeakMap(), _priority = new WeakMap(), _score = new WeakMap(), _preferredLayers = new WeakMap(), _currentLayers = new WeakMap(), _observer = new WeakMap();
