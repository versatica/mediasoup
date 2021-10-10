"use strict";
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
        // Closed flag.
        this._closed = false;
        // Paused flag.
        this._paused = false;
        // Current score.
        this._score = [];
        // Observer instance.
        this._observer = new EnhancedEventEmitter_1.EnhancedEventEmitter();
        logger.debug('constructor()');
        this._internal = internal;
        this._data = data;
        this._channel = channel;
        this._payloadChannel = payloadChannel;
        this._appData = appData;
        this._paused = paused;
        this._handleWorkerNotifications();
    }
    /**
     * Producer id.
     */
    get id() {
        return this._internal.producerId;
    }
    /**
     * Whether the Producer is closed.
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
     * Producer type.
     */
    get type() {
        return this._data.type;
    }
    /**
     * Consumable RTP parameters.
     *
     * @private
     */
    get consumableRtpParameters() {
        return this._data.consumableRtpParameters;
    }
    /**
     * Whether the Producer is paused.
     */
    get paused() {
        return this._paused;
    }
    /**
     * Producer score list.
     */
    get score() {
        return this._score;
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
     * @emits score - (score: ProducerScore[])
     * @emits videoorientationchange - (videoOrientation: ProducerVideoOrientation)
     * @emits trace - (trace: ProducerTraceEventData)
     */
    get observer() {
        return this._observer;
    }
    /**
     * Close the Producer.
     */
    close() {
        if (this._closed)
            return;
        logger.debug('close()');
        this._closed = true;
        // Remove notification subscriptions.
        this._channel.removeAllListeners(this._internal.producerId);
        this._payloadChannel.removeAllListeners(this._internal.producerId);
        this._channel.request('producer.close', this._internal)
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
        this._channel.removeAllListeners(this._internal.producerId);
        this._payloadChannel.removeAllListeners(this._internal.producerId);
        this.safeEmit('transportclose');
        // Emit observer event.
        this._observer.safeEmit('close');
    }
    /**
     * Dump Producer.
     */
    async dump() {
        logger.debug('dump()');
        return this._channel.request('producer.dump', this._internal);
    }
    /**
     * Get Producer stats.
     */
    async getStats() {
        logger.debug('getStats()');
        return this._channel.request('producer.getStats', this._internal);
    }
    /**
     * Pause the Producer.
     */
    async pause() {
        logger.debug('pause()');
        const wasPaused = this._paused;
        await this._channel.request('producer.pause', this._internal);
        this._paused = true;
        // Emit observer event.
        if (!wasPaused)
            this._observer.safeEmit('pause');
    }
    /**
     * Resume the Producer.
     */
    async resume() {
        logger.debug('resume()');
        const wasPaused = this._paused;
        await this._channel.request('producer.resume', this._internal);
        this._paused = false;
        // Emit observer event.
        if (wasPaused)
            this._observer.safeEmit('resume');
    }
    /**
     * Enable 'trace' event.
     */
    async enableTraceEvent(types = []) {
        logger.debug('enableTraceEvent()');
        const reqData = { types };
        await this._channel.request('producer.enableTraceEvent', this._internal, reqData);
    }
    /**
     * Send RTP packet (just valid for Producers created on a DirectTransport).
     */
    send(rtpPacket) {
        if (!Buffer.isBuffer(rtpPacket)) {
            throw new TypeError('rtpPacket must be a Buffer');
        }
        this._payloadChannel.notify('producer.send', this._internal, undefined, rtpPacket);
    }
    _handleWorkerNotifications() {
        this._channel.on(this._internal.producerId, (event, data) => {
            switch (event) {
                case 'score':
                    {
                        const score = data;
                        this._score = score;
                        this.safeEmit('score', score);
                        // Emit observer event.
                        this._observer.safeEmit('score', score);
                        break;
                    }
                case 'videoorientationchange':
                    {
                        const videoOrientation = data;
                        this.safeEmit('videoorientationchange', videoOrientation);
                        // Emit observer event.
                        this._observer.safeEmit('videoorientationchange', videoOrientation);
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
    }
}
exports.Producer = Producer;
