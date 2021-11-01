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
var _internal, _data, _channel, _payloadChannel, _closed, _appData, _paused, _score, _observer;
Object.defineProperty(exports, "__esModule", { value: true });
const Logger_1 = require("./Logger");
const EnhancedEventEmitter_1 = require("./EnhancedEventEmitter");
const logger = new Logger_1.Logger('Producer');
class Producer extends EnhancedEventEmitter_1.EnhancedEventEmitter {
    /**
     * @private
     * @emits transportclose
     * @emits score - (score: ProducerScore[])
     * @emits videoorientationchange - (videoOrientation: ProducerVideoOrientation)
     * @emits trace - (trace: ProducerTraceEventData)
     * @emits @close
     */
    constructor({ internal, data, channel, payloadChannel, appData, paused }) {
        super();
        // Internal data.
        _internal.set(this, void 0);
        // Producer data.
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
        // Current score.
        _score.set(this, []);
        // Observer instance.
        _observer.set(this, new EnhancedEventEmitter_1.EnhancedEventEmitter());
        logger.debug('constructor()');
        __classPrivateFieldSet(this, _internal, internal);
        __classPrivateFieldSet(this, _data, data);
        __classPrivateFieldSet(this, _channel, channel);
        __classPrivateFieldSet(this, _payloadChannel, payloadChannel);
        __classPrivateFieldSet(this, _appData, appData);
        __classPrivateFieldSet(this, _paused, paused);
        this.handleWorkerNotifications();
    }
    /**
     * Producer id.
     */
    get id() {
        return __classPrivateFieldGet(this, _internal).producerId;
    }
    /**
     * Whether the Producer is closed.
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
     * Producer type.
     */
    get type() {
        return __classPrivateFieldGet(this, _data).type;
    }
    /**
     * Consumable RTP parameters.
     *
     * @private
     */
    get consumableRtpParameters() {
        return __classPrivateFieldGet(this, _data).consumableRtpParameters;
    }
    /**
     * Whether the Producer is paused.
     */
    get paused() {
        return __classPrivateFieldGet(this, _paused);
    }
    /**
     * Producer score list.
     */
    get score() {
        return __classPrivateFieldGet(this, _score);
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
     * @emits score - (score: ProducerScore[])
     * @emits videoorientationchange - (videoOrientation: ProducerVideoOrientation)
     * @emits trace - (trace: ProducerTraceEventData)
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
     * Close the Producer.
     */
    close() {
        if (__classPrivateFieldGet(this, _closed))
            return;
        logger.debug('close()');
        __classPrivateFieldSet(this, _closed, true);
        // Remove notification subscriptions.
        __classPrivateFieldGet(this, _channel).removeAllListeners(__classPrivateFieldGet(this, _internal).producerId);
        __classPrivateFieldGet(this, _payloadChannel).removeAllListeners(__classPrivateFieldGet(this, _internal).producerId);
        __classPrivateFieldGet(this, _channel).request('producer.close', __classPrivateFieldGet(this, _internal))
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
        __classPrivateFieldGet(this, _channel).removeAllListeners(__classPrivateFieldGet(this, _internal).producerId);
        __classPrivateFieldGet(this, _payloadChannel).removeAllListeners(__classPrivateFieldGet(this, _internal).producerId);
        this.safeEmit('transportclose');
        // Emit observer event.
        __classPrivateFieldGet(this, _observer).safeEmit('close');
    }
    /**
     * Dump Producer.
     */
    async dump() {
        logger.debug('dump()');
        return __classPrivateFieldGet(this, _channel).request('producer.dump', __classPrivateFieldGet(this, _internal));
    }
    /**
     * Get Producer stats.
     */
    async getStats() {
        logger.debug('getStats()');
        return __classPrivateFieldGet(this, _channel).request('producer.getStats', __classPrivateFieldGet(this, _internal));
    }
    /**
     * Pause the Producer.
     */
    async pause() {
        logger.debug('pause()');
        const wasPaused = __classPrivateFieldGet(this, _paused);
        await __classPrivateFieldGet(this, _channel).request('producer.pause', __classPrivateFieldGet(this, _internal));
        __classPrivateFieldSet(this, _paused, true);
        // Emit observer event.
        if (!wasPaused)
            __classPrivateFieldGet(this, _observer).safeEmit('pause');
    }
    /**
     * Resume the Producer.
     */
    async resume() {
        logger.debug('resume()');
        const wasPaused = __classPrivateFieldGet(this, _paused);
        await __classPrivateFieldGet(this, _channel).request('producer.resume', __classPrivateFieldGet(this, _internal));
        __classPrivateFieldSet(this, _paused, false);
        // Emit observer event.
        if (wasPaused)
            __classPrivateFieldGet(this, _observer).safeEmit('resume');
    }
    /**
     * Enable 'trace' event.
     */
    async enableTraceEvent(types = []) {
        logger.debug('enableTraceEvent()');
        const reqData = { types };
        await __classPrivateFieldGet(this, _channel).request('producer.enableTraceEvent', __classPrivateFieldGet(this, _internal), reqData);
    }
    /**
     * Send RTP packet (just valid for Producers created on a DirectTransport).
     */
    send(rtpPacket) {
        if (!Buffer.isBuffer(rtpPacket)) {
            throw new TypeError('rtpPacket must be a Buffer');
        }
        __classPrivateFieldGet(this, _payloadChannel).notify('producer.send', __classPrivateFieldGet(this, _internal), undefined, rtpPacket);
    }
    handleWorkerNotifications() {
        __classPrivateFieldGet(this, _channel).on(__classPrivateFieldGet(this, _internal).producerId, (event, data) => {
            switch (event) {
                case 'score':
                    {
                        const score = data;
                        __classPrivateFieldSet(this, _score, score);
                        this.safeEmit('score', score);
                        // Emit observer event.
                        __classPrivateFieldGet(this, _observer).safeEmit('score', score);
                        break;
                    }
                case 'videoorientationchange':
                    {
                        const videoOrientation = data;
                        this.safeEmit('videoorientationchange', videoOrientation);
                        // Emit observer event.
                        __classPrivateFieldGet(this, _observer).safeEmit('videoorientationchange', videoOrientation);
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
    }
}
exports.Producer = Producer;
_internal = new WeakMap(), _data = new WeakMap(), _channel = new WeakMap(), _payloadChannel = new WeakMap(), _closed = new WeakMap(), _appData = new WeakMap(), _paused = new WeakMap(), _score = new WeakMap(), _observer = new WeakMap();
