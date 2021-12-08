"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.Producer = void 0;
const Logger_1 = require("./Logger");
const EnhancedEventEmitter_1 = require("./EnhancedEventEmitter");
const logger = new Logger_1.Logger('Producer');
class Producer extends EnhancedEventEmitter_1.EnhancedEventEmitter {
    // Internal data.
    #internal;
    // Producer data.
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
    // Current score.
    #score = [];
    // Observer instance.
    #observer = new EnhancedEventEmitter_1.EnhancedEventEmitter();
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
        logger.debug('constructor()');
        this.#internal = internal;
        this.#data = data;
        this.#channel = channel;
        this.#payloadChannel = payloadChannel;
        this.#appData = appData;
        this.#paused = paused;
        this.handleWorkerNotifications();
    }
    /**
     * Producer id.
     */
    get id() {
        return this.#internal.producerId;
    }
    /**
     * Whether the Producer is closed.
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
     * Producer type.
     */
    get type() {
        return this.#data.type;
    }
    /**
     * Consumable RTP parameters.
     *
     * @private
     */
    get consumableRtpParameters() {
        return this.#data.consumableRtpParameters;
    }
    /**
     * Whether the Producer is paused.
     */
    get paused() {
        return this.#paused;
    }
    /**
     * Producer score list.
     */
    get score() {
        return this.#score;
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
     * @emits score - (score: ProducerScore[])
     * @emits videoorientationchange - (videoOrientation: ProducerVideoOrientation)
     * @emits trace - (trace: ProducerTraceEventData)
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
     * Close the Producer.
     */
    close() {
        if (this.#closed)
            return;
        logger.debug('close()');
        this.#closed = true;
        // Remove notification subscriptions.
        this.#channel.removeAllListeners(this.#internal.producerId);
        this.#payloadChannel.removeAllListeners(this.#internal.producerId);
        this.#channel.request('producer.close', this.#internal)
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
        this.#channel.removeAllListeners(this.#internal.producerId);
        this.#payloadChannel.removeAllListeners(this.#internal.producerId);
        this.safeEmit('transportclose');
        // Emit observer event.
        this.#observer.safeEmit('close');
    }
    /**
     * Dump Producer.
     */
    async dump() {
        logger.debug('dump()');
        return this.#channel.request('producer.dump', this.#internal);
    }
    /**
     * Get Producer stats.
     */
    async getStats() {
        logger.debug('getStats()');
        return this.#channel.request('producer.getStats', this.#internal);
    }
    /**
     * Pause the Producer.
     */
    async pause() {
        logger.debug('pause()');
        const wasPaused = this.#paused;
        await this.#channel.request('producer.pause', this.#internal);
        this.#paused = true;
        // Emit observer event.
        if (!wasPaused)
            this.#observer.safeEmit('pause');
    }
    /**
     * Resume the Producer.
     */
    async resume() {
        logger.debug('resume()');
        const wasPaused = this.#paused;
        await this.#channel.request('producer.resume', this.#internal);
        this.#paused = false;
        // Emit observer event.
        if (wasPaused)
            this.#observer.safeEmit('resume');
    }
    /**
     * Enable 'trace' event.
     */
    async enableTraceEvent(types = []) {
        logger.debug('enableTraceEvent()');
        const reqData = { types };
        await this.#channel.request('producer.enableTraceEvent', this.#internal, reqData);
    }
    /**
     * Send RTP packet (just valid for Producers created on a DirectTransport).
     */
    send(rtpPacket) {
        if (!Buffer.isBuffer(rtpPacket)) {
            throw new TypeError('rtpPacket must be a Buffer');
        }
        this.#payloadChannel.notify('producer.send', this.#internal, undefined, rtpPacket);
    }
    handleWorkerNotifications() {
        this.#channel.on(this.#internal.producerId, (event, data) => {
            switch (event) {
                case 'score':
                    {
                        const score = data;
                        this.#score = score;
                        this.safeEmit('score', score);
                        // Emit observer event.
                        this.#observer.safeEmit('score', score);
                        break;
                    }
                case 'videoorientationchange':
                    {
                        const videoOrientation = data;
                        this.safeEmit('videoorientationchange', videoOrientation);
                        // Emit observer event.
                        this.#observer.safeEmit('videoorientationchange', videoOrientation);
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
    }
}
exports.Producer = Producer;
